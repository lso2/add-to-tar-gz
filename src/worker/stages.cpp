// stages.cpp: two 7-Zip processes joined by pipes, waited on with cancel support.
// Globals: none beyond stages.h.
// Doc: docs/notes/src/worker/stages.md
#include "stages.h"
#include "child.h"
#include "errors.h"
#include "pipes.h"
#include <thread>

namespace {

void WaitForBoth(ChildProcess& first, ChildProcess& second, Progress& progress, bool& cancelled)
{
    HANDLE handles[2] = {first.Process(), second.Process()};
    for (;;) {
        const DWORD waited = WaitForMultipleObjects(2, handles, TRUE, 100);
        if (waited != WAIT_TIMEOUT) {
            return;
        }
        if (progress.cancel.load() && !cancelled) {
            cancelled = true;
            first.Terminate();
            second.Terminate();
        }
    }
}

void Collect(ChildProcess& child, StageOutcome& outcome)
{
    if (!outcome.started) {
        return;
    }
    outcome.exitCode = child.ExitCode();
    outcome.messages = child.ErrorText();
}

}  // namespace

DWORD ChainOutcome::WorstExitCode() const
{
    if (!first.started || !second.started) {
        return 255;
    }
    return first.exitCode > second.exitCode ? first.exitCode : second.exitCode;
}

ChainOutcome RunAddChain(const std::wstring& sevenZip, const std::vector<std::wstring>& tarArgs,
                         const std::vector<std::wstring>& gzipArgs, HANDLE outputFile, Progress& progress)
{
    ChainOutcome outcome;
    Pipe tarToRelay;
    Pipe relayToGzip;
    HANDLE nulInput = nullptr;
    HANDLE outputForChild = nullptr;
    std::wstring setupError;
    bool ready = MakePipe(tarToRelay, false, true, setupError) && MakePipe(relayToGzip, true, false, setupError);
    if (ready) {
        nulInput = OpenNul(setupError);
        ready = nulInput != nullptr;
    }
    if (ready && !DuplicateHandle(GetCurrentProcess(), outputFile, GetCurrentProcess(), &outputForChild, 0, TRUE,
                                  DUPLICATE_SAME_ACCESS)) {
        setupError = L"DuplicateHandle failed: " + WindowsErrorText(GetLastError());
        ready = false;
    }

    ChildProcess tar;
    ChildProcess gzip;
    if (!ready) {
        outcome.first.startError = setupError;
    } else {
        outcome.first.started = tar.Start(sevenZip, tarArgs, std::wstring(), nulInput, tarToRelay.write, outcome.first.startError);
        if (outcome.first.started) {
            outcome.second.started =
                gzip.Start(sevenZip, gzipArgs, std::wstring(), relayToGzip.read, outputForChild, outcome.second.startError);
        }
    }
    // The children have their own copies at this point, and closing ours lets each pipe end with its child.
    CloseIfOpen(tarToRelay.write);
    CloseIfOpen(relayToGzip.read);
    CloseIfOpen(outputForChild);
    CloseIfOpen(nulInput);

    if (!outcome.first.started || !outcome.second.started) {
        CloseIfOpen(tarToRelay.read);
        CloseIfOpen(relayToGzip.write);
        tar.Terminate();
        gzip.Terminate();
        Collect(tar, outcome.first);
        Collect(gzip, outcome.second);
        return outcome;
    }

    std::thread relay([&] {
        outcome.relayError = RelayBytes(tarToRelay.read, relayToGzip.write, progress.doneBytes, progress.cancel);
        // Closing both ends wakes a tar stage blocked on a full pipe and gives the gzip stage its end of input.
        CloseIfOpen(tarToRelay.read);
        CloseIfOpen(relayToGzip.write);
    });
    WaitForBoth(tar, gzip, progress, outcome.cancelled);
    relay.join();
    Collect(tar, outcome.first);
    Collect(gzip, outcome.second);
    return outcome;
}

ChainOutcome RunExtractChain(const std::wstring& sevenZip, HANDLE archiveFile, const std::vector<std::wstring>& gunzipArgs,
                             const std::vector<std::wstring>& untarArgs, Progress& progress)
{
    ChainOutcome outcome;
    Pipe relayToGunzip;
    Pipe gunzipToUntar;
    HANDLE nulOutput = nullptr;
    std::wstring setupError;
    bool ready = MakePipe(relayToGunzip, true, false, setupError) && MakePipe(gunzipToUntar, true, true, setupError);
    if (ready) {
        nulOutput = OpenNul(setupError);
        ready = nulOutput != nullptr;
    }

    ChildProcess gunzip;
    ChildProcess untar;
    if (!ready) {
        outcome.first.startError = setupError;
    } else {
        outcome.first.started = gunzip.Start(sevenZip, gunzipArgs, std::wstring(), relayToGunzip.read, gunzipToUntar.write,
                                             outcome.first.startError);
        if (outcome.first.started) {
            outcome.second.started =
                untar.Start(sevenZip, untarArgs, std::wstring(), gunzipToUntar.read, nulOutput, outcome.second.startError);
        }
    }
    CloseIfOpen(relayToGunzip.read);
    CloseIfOpen(gunzipToUntar.write);
    CloseIfOpen(gunzipToUntar.read);
    CloseIfOpen(nulOutput);

    if (!outcome.first.started || !outcome.second.started) {
        // Closing the relay end gives a started gunzip stage its end of input.
        CloseIfOpen(relayToGunzip.write);
        gunzip.Terminate();
        untar.Terminate();
        Collect(gunzip, outcome.first);
        Collect(untar, outcome.second);
        return outcome;
    }

    std::thread relay([&] {
        outcome.relayError = RelayBytes(archiveFile, relayToGunzip.write, progress.doneBytes, progress.cancel);
        CloseIfOpen(relayToGunzip.write);
    });
    WaitForBoth(gunzip, untar, progress, outcome.cancelled);
    relay.join();
    Collect(gunzip, outcome.first);
    Collect(untar, outcome.second);
    return outcome;
}

std::wstring DescribeChain(const ChainOutcome& outcome, const wchar_t* firstName, const wchar_t* secondName)
{
    std::wstring text;
    const StageOutcome* stages[2] = {&outcome.first, &outcome.second};
    const wchar_t* names[2] = {firstName, secondName};
    for (int i = 0; i < 2; ++i) {
        const StageOutcome& stage = *stages[i];
        if (!stage.started) {
            if (!stage.startError.empty()) {
                text += L"7-Zip " + std::wstring(names[i]) + L" stage did not start: " + stage.startError + L"\r\n";
            }
            continue;
        }
        text += L"7-Zip " + std::wstring(names[i]) + L" stage exit code " + std::to_wstring(stage.exitCode);
        text += stage.messages.empty() ? L", no messages.\r\n" : L":\r\n" + stage.messages + L"\r\n";
    }
    if (outcome.relayError != 0 && outcome.relayError != ERROR_CANCELLED) {
        text += L"Passing data between the stages stopped: " + WindowsErrorText(outcome.relayError) + L"\r\n";
    }
    return text;
}
