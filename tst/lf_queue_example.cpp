#include "../src/lf_queue.hpp"
#include "../src/thread_utils.hpp"

#include <iostream>

struct MyStruct 
{
    int d_[3];
};

auto consumeFunction(common::LFQueue<MyStruct> *lfq)
{
    using namespace std::literals::chrono_literals;
    std::this_thread::sleep_for(5s);
    
    while(lfq->size())
    {
        const auto d = lfq->getNextToRead();
        lfq->updateReadIndex();
        std::cout << "ConsumeFunction read element: " << d->d_[0] << "," << d->d_[1] << "," << d->d_[2] << " lfq-size: " << lfq->size() << std::endl;
        std::this_thread::sleep_for(1s);
    }
    std::cout << "consumeFunction exiting." << std::endl;
}

int main(int, char **argv)
{
    common::LFQueue<MyStruct> lfq(20);

    auto ct = common::createAndStartThread(-1, "",consumeFunction, &lfq);

    for (auto i = 0; i < 50; i++) 
    {
        const MyStruct d{i, i * 10, i * 100};
        *(lfq.getNextToWriteTo()) = d;
        lfq.updateWriteIndex();
        std::cout << "main constructed element: " << d.d_[0] << "," << d.d_[1] << "," << d.d_[2] << " lfq-size:" << lfq.size() << std::endl;

        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(1s);
    }
    
    ct->join();
    std::cout << "main exiting." << std::endl;
    return 0;
}