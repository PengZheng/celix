/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "celix/BundleActivator.h"
#include "examples/ICalc.h"

class SimpleConsumer {
public:
    ~SimpleConsumer() {
        stop();
    }

    SimpleConsumer() = default;
    SimpleConsumer(SimpleConsumer&&) = delete;
    SimpleConsumer& operator=(SimpleConsumer&&) = delete;
    SimpleConsumer(const SimpleConsumer&) = delete;
    SimpleConsumer& operator=(const SimpleConsumer&) = delete;

    void start() {
        std::lock_guard<std::mutex> lck{mutex};
        calcThread = std::thread{&SimpleConsumer::run, this};
    }

    void stop() {
        active = false;
        std::lock_guard<std::mutex> lck{mutex};
        if (calcThread.joinable()) {
            calcThread.join();
        }
    }

    void setCalc(std::shared_ptr<examples::ICalc> _calc) {
        std::lock_guard<std::mutex> lck{mutex};
        calc = std::move(_calc);
    }
private:
    void run() {
        std::unique_lock<std::mutex> lck{mutex,  std::defer_lock};
        int count = 1;
        while (active) {
            lck.lock();
            auto localCalc = calc;
            lck.unlock();

            /*
             * note it is safe to use the localCalc outside a mutex,
             * because the shared_prt count will ensure the service cannot be unregistered while in use.
             */
            if (localCalc) {
                std::cout << "Calc result for input " << count << " is " << localCalc->calc(count) << std::endl;
            } else {
                std::cout << "Calc service not available!" << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::seconds{2});
            ++count;
        }
    }

    std::atomic<bool> active{true};

    std::mutex mutex{}; //protects below
    std::shared_ptr<examples::ICalc> calc{};
    std::thread calcThread{};
};

class SimpleConsumerBundleActivator {
public:
    explicit SimpleConsumerBundleActivator(const std::shared_ptr<celix::BundleContext>& ctx) :
            tracker{createTracker(ctx)} {
        consumer.start();
    }
private:
    std::shared_ptr<celix::GenericServiceTracker> createTracker(const std::shared_ptr<celix::BundleContext>& ctx) {
        return ctx->trackServices<examples::ICalc>()
                .addSetCallback(std::bind(&SimpleConsumer::setCalc, &consumer, std::placeholders::_1))
                .build();
    }

    const std::shared_ptr<celix::GenericServiceTracker> tracker;
    SimpleConsumer consumer{};
};

CELIX_GEN_CXX_BUNDLE_ACTIVATOR(SimpleConsumerBundleActivator)