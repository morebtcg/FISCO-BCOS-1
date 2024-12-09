#pragma once

#include "bcos-utilities/Common.h"
#include <indicators/block_progress_bar.hpp>
#include <indicators/cursor_control.hpp>
#include <indicators/multi_progress.hpp>

namespace bcos::sample
{

bcos::bytes getContractBin();
std::string_view getContractABI();
long currentTime();

class Collector
{
private:
    long m_startTime;
    long m_endTime;
    int m_count;
    std::string m_title;
    bool m_showProgressBar;

    indicators::BlockProgressBar m_sendProgressBar;
    indicators::BlockProgressBar m_receiveProgressBar;
    indicators::MultiProgress<indicators::BlockProgressBar, 1> m_progressBar;

    long m_sendElapsed = std::numeric_limits<long>::max();
    std::atomic_long m_allTimeCost = 0;
    std::atomic_int m_sended = 0;
    std::atomic_int m_finished = 0;
    std::atomic_int m_failed = 0;

public:
    Collector(int count, std::string title, bool showProgress = true);

    void finishSend();
    void send(bool success, long elapsed);
    void receive(bool success, long elapsed);
    void report();
};

}  // namespace bcos::sample