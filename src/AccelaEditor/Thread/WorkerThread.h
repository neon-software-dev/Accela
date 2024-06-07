/*
 * SPDX-FileCopyrightText: 2024 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef ACCELAEDITOR_THREAD_WORKERTHREAD_H
#define ACCELAEDITOR_THREAD_WORKERTHREAD_H

#include <QThread>
#include <functional>

namespace Accela
{
    /**
     * Helper class which runs logic on a Qt thread and returns a result via signal as well as a
     * manual GetResult() method which can be called.
     *
     * Sample usage:
     *
     * auto workerThread = WorkerThread::Create<bool>(this, [](){
     *    // {do thread work here ..}
     *
     *    // return result
     *    return true;
     * });
     *
     * connect(workerThread, &WorkerThread::OnResult, this, &MyObject::OnWorkResult);
     * connect(workerThread, &WorkerThread::finished, workerThread, &QObject::deleteLater);
     *
     * workerThread->start();
     */
    class WorkerThread : public QThread
    {
        Q_OBJECT

        public:

            struct ResultHolder
            {
                using Ptr = std::shared_ptr<ResultHolder>;

                virtual ~ResultHolder() = default;
            };

            template <typename ResultType>
            struct TypedResultHolder : public ResultHolder
            {
                using Ptr = std::shared_ptr<TypedResultHolder<ResultType>>;

                explicit TypedResultHolder(ResultType _result)
                    : result(std::move(_result))
                { }

                ResultType result;
            };

            template <typename ResultType>
            static ResultType ResultAs(const ResultHolder::Ptr& resultHolder)
            {
                return std::dynamic_pointer_cast<TypedResultHolder<ResultType>>(resultHolder)->result;
            }

            struct WorkControl
            {
                bool isCancelled{false};
            };

        private:

            struct Work
            {
                using UPtr = std::unique_ptr<Work>;

                virtual ~Work() = default;

                [[nodiscard]] virtual ResultHolder::Ptr Execute(const WorkControl& workControl) = 0;
            };

            template <typename ResultType>
            struct SpecificWork : public Work
            {
                explicit SpecificWork(std::function<ResultType(const WorkControl&)> logic)
                    : m_logic(std::move(logic))
                { }

                [[nodiscard]] ResultHolder::Ptr Execute(const WorkControl& workControl) override
                {
                    return std::make_shared<TypedResultHolder<ResultType>>(std::invoke(m_logic, workControl));
                }

                private:

                    std::function<ResultType(const WorkControl&)> m_logic;
            };

        public:

            template <typename ResultType>
            static WorkerThread* Create(QObject* pParent, std::function<ResultType(const WorkControl&)> logic)
            {
                return new WorkerThread(pParent, std::make_unique<SpecificWork<ResultType>>(std::move(logic)));
            }

        public:

            WorkerThread(QObject *pParent, Work::UPtr work)
                : QThread(pParent)
                , m_work(std::move(work))
            { }

            [[nodiscard]] ResultHolder::Ptr GetResult() const noexcept { return m_result; }

        signals:

            void OnResult(ResultHolder::Ptr result);

        public slots:

            void OnCancelled()
            {
                m_workControl.isCancelled = true;
            };

        protected:

            void run() override
            {
                m_result = m_work->Execute(m_workControl);
                emit OnResult(m_result);
            }

        private:

            Work::UPtr m_work;
            WorkControl m_workControl{};

            ResultHolder::Ptr m_result;
    };
}

#endif //ACCELAEDITOR_THREAD_WORKERTHREAD_H
