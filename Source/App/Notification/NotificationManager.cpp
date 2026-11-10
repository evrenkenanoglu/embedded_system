/** @file       NotificationManager.cpp
 *  @brief      Publish/subscribe hub for user notifications with a state snapshot.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       01/11/2026
 */

/** INCLUDES ******************************************************************/
#include "App/Notification/NotificationManager.hpp"

#include "System/LogHandler.h"
#include "System/error_macros.h"

/** LOCAL FUNCTIONS ***********************************************************/
namespace
{
    constexpr size_t   kDirtyWordCount     = kNotificationTotalIds / 32;
    constexpr uint16_t kSequenceHalfRange  = 0x7FFF;
    const char* const  kDispatcherTaskName = "NotifyDispatch";

    bool isAfterBaseline(uint16_t sequence, uint16_t baseline)
    {
        uint16_t distance = static_cast<uint16_t>(sequence - baseline);
        return (distance != 0) && (distance <= kSequenceHalfRange);
    }
} // namespace

/** LIFECYCLE *****************************************************************/

NotificationManager::NotificationManager()
    : _dispatcherTask(nullptr)
    , _dispatchQueue(nullptr)
    , _stateMutex(nullptr)
    , _tableMutex(nullptr)
    , _snapshot{}
    , _dirtyBits{}
    , _slots{}
    , _sequence(0)
    , _droppedEvents(0)
    , _stopRequested(false)
    , _pauseRequested(false)
    , _dispatcherExited(false)
{
    setState(State::INITIALIZED);
}

NotificationManager::~NotificationManager()
{
    if ((getState() == State::RUNNING) || (getState() == State::PAUSED))
    {
        stop();
    }
}

sys_error_t NotificationManager::start()
{
    RETURN_IF_ERROR(
        ((getState() == State::RUNNING) || (getState() == State::PAUSED)), // Expression
        ERROR_INVALID_STATE,                                               // Error code
        SYS_LOG_E("Notification manager already started")                  // Error message
    );

    _stopRequested.store(false);
    _pauseRequested.store(false);
    _dispatcherExited.store(false);

    /// Create the locks and the dispatch queue.
    _stateMutex    = xSemaphoreCreateMutex();
    _tableMutex    = xSemaphoreCreateMutex();
    _dispatchQueue = xQueueCreate(kDispatchQueueLength, sizeof(user_notification_t));
    RETURN_IF_ERROR(
        ((_stateMutex == nullptr) || (_tableMutex == nullptr) || (_dispatchQueue == nullptr)), // Expression
        ERROR_OUT_OF_MEMORY,                                                                   // Error code
        _releaseResources(),                                                                   // Cleanup
        SYS_LOG_E("Failed to create notification resources")                                   // Error message
    );

    /// Launch the dispatcher that owns every subscriber callback.
    RETURN_IF_ERROR(
        (xTaskCreate(_dispatcherEntry, kDispatcherTaskName, kDispatcherStackBytes, this, kDispatcherPriority, &_dispatcherTask) != pdPASS), // Expression
        ERROR_FAIL,                                                                                                                         // Error code
        _releaseResources(),                                                                                                                // Cleanup
        SYS_LOG_E("Failed to create notification dispatcher task")                                                                          // Error message
    );

    setState(State::RUNNING);
    return ERROR_SUCCESS;
}

sys_error_t NotificationManager::stop()
{
    if ((getState() != State::RUNNING) && (getState() != State::PAUSED))
    {
        return ERROR_SUCCESS;
    }

    RETURN_IF_ERROR(
        (xTaskGetCurrentTaskHandle() == _dispatcherTask),   // Expression
        ERROR_INVALID_STATE,                                // Error code
        SYS_LOG_E("stop() called from the dispatcher task") // Error message
    );

    /// Request exit and wait a bounded time for the dispatcher loop to finish.
    _stopRequested.store(true);
    sys_error_t      result    = ERROR_SUCCESS;
    const TickType_t startTick = xTaskGetTickCount();
    while (!_dispatcherExited.load() && ((xTaskGetTickCount() - startTick) < pdMS_TO_TICKS(kDrainTimeoutMs)))
    {
        vTaskDelay(1);
    }

    if (!_dispatcherExited.load())
    {
        SYS_LOG_E("Notification dispatcher did not exit in time");
        result = ERROR_TIMEOUT;
    }

    /// Delete the dispatcher task; once its loop exits it is parked in vTaskSuspend.
    if (_dispatcherTask != nullptr)
    {
        vTaskDelete(_dispatcherTask);
        _dispatcherTask = nullptr;
    }

    /// Drop all subscriptions so a restart requires explicit re-subscription.
    if (xSemaphoreTake(_tableMutex, pdMS_TO_TICKS(kMutexTimeoutMs)) == pdTRUE)
    {
        for (size_t i = 0; i < kMaxSubscribers; ++i)
        {
            _clearSlot(i);
        }
        xSemaphoreGive(_tableMutex);
    }
    else
    {
        SYS_LOG_E("Notification table lock timed out during stop");
        result = ERROR_TIMEOUT;
    }

    _releaseResources();
    setState(State::STOPPED);
    return result;
}

sys_error_t NotificationManager::pause()
{
    RETURN_IF_ERROR(
        (getState() != State::RUNNING),                  // Expression
        ERROR_INVALID_STATE,                             // Error code
        SYS_LOG_E("Notification manager is not running") // Error message
    );

    _pauseRequested.store(true);
    setState(State::PAUSED);
    return ERROR_SUCCESS;
}

sys_error_t NotificationManager::resume()
{
    RETURN_IF_ERROR(
        (getState() != State::PAUSED),                  // Expression
        ERROR_INVALID_STATE,                            // Error code
        SYS_LOG_E("Notification manager is not paused") // Error message
    );

    _pauseRequested.store(false);
    setState(State::RUNNING);
    return ERROR_SUCCESS;
}

/** PUBLIC API ****************************************************************/

sys_error_t NotificationManager::publish(const user_notification_t& notification)
{
    RETURN_IF_ERROR(
        (notification.id == static_cast<notification_id_t>(framework_notification_id_t::NONE)), // Expression
        ERROR_INVALID_ARG,                                                                      // Error code
        SYS_LOG_E("Notification id is NONE")                                                    // Error message
    );

    RETURN_IF_ERROR(
        (notification.category == static_cast<notification_category_t>(0)), // Expression
        ERROR_INVALID_ARG,                                                  // Error code
        SYS_LOG_E("Notification category is not set")                       // Error message
    );

    RETURN_IF_ERROR(
        ((notification.kind != notification_kind_t::STATE) && (notification.kind != notification_kind_t::EVENT)), // Expression
        ERROR_INVALID_ARG,                                                                                        // Error code
        SYS_LOG_E("Notification kind is invalid")                                                                 // Error message
    );

    RETURN_IF_ERROR(
        ((_stateMutex == nullptr) || (_dispatchQueue == nullptr)), // Expression
        ERROR_INVALID_STATE,                                       // Error code
        SYS_LOG_E("Notification manager is not started")           // Error message
    );

    RETURN_IF_ERROR(
        (xSemaphoreTake(_stateMutex, pdMS_TO_TICKS(kMutexTimeoutMs)) != pdTRUE), // Expression
        ERROR_TIMEOUT,                                                           // Error code
        SYS_LOG_E("Notification state lock timed out")                           // Error message
    );

    /// Assign the sequence and update the snapshot for STATE notifications.
    user_notification_t stamped = notification;
    stamped.sequence            = ++_sequence;

    const size_t id = notification.id;
    if (stamped.kind == notification_kind_t::STATE)
    {
        _snapshot[id].present      = stamped.active;
        _snapshot[id].notification = stamped;
    }

    /// Enqueue without blocking. Under the state lock so queue order matches sequence order.
    const bool queued = (xQueueSend(_dispatchQueue, &stamped, 0) == pdTRUE);
    if (!queued && (stamped.kind == notification_kind_t::STATE))
    {
        _dirtyBits[id / 32] |= (1UL << (id % 32));
    }
    xSemaphoreGive(_stateMutex);

    if (!queued && (stamped.kind == notification_kind_t::EVENT))
    {
        _droppedEvents.fetch_add(1);
        SYS_LOG_W("Notification event dropped, dispatch queue full");
    }

    return ERROR_SUCCESS;
}

sys_error_t NotificationManager::publishState(notification_id_t id, notification_category_t category, notification_severity_t severity, bool active)
{
    user_notification_t notification{};
    notification.id          = id;
    notification.category    = category;
    notification.kind        = notification_kind_t::STATE;
    notification.severity    = severity;
    notification.active      = active;
    notification.payload.u32 = 0U;

    const sys_error_t error = publish(notification);
    if (error != ERROR_SUCCESS)
    {
        SYS_LOG_E("Notification state publish failed (error=%d)", static_cast<int>(error));
    }
    return error;
}

sys_error_t NotificationManager::subscribe(INotificationSubscriber& subscriber, notification_filter_t filter)
{
    RETURN_IF_ERROR(
        ((_tableMutex == nullptr) || (_stateMutex == nullptr)), // Expression
        ERROR_INVALID_STATE,                                    // Error code
        SYS_LOG_E("Notification manager is not started")        // Error message
    );

    RETURN_IF_ERROR(
        (xSemaphoreTake(_tableMutex, pdMS_TO_TICKS(kMutexTimeoutMs)) != pdTRUE), // Expression
        ERROR_TIMEOUT,                                                           // Error code
        SYS_LOG_E("Notification table lock timed out")                           // Error message
    );

    /// Reject duplicates and locate the first free slot.
    size_t      existingIndex = kMaxSubscribers;
    size_t      freeIndex     = kMaxSubscribers;
    sys_error_t result        = ERROR_OUT_OF_MEMORY;

    if (_findSlot(subscriber, existingIndex) == ERROR_SUCCESS)
    {
        result = ERROR_INVALID_ARG;
    }
    else
    {
        for (size_t i = 0; (i < kMaxSubscribers) && (freeIndex == kMaxSubscribers); ++i)
        {
            if (!_slots[i].active)
            {
                freeIndex = i;
            }
        }
    }

    if ((result != ERROR_INVALID_ARG) && (freeIndex != kMaxSubscribers))
    {
        /// Capture the baseline so queued messages already reflected in the snapshot are skipped.
        if (xSemaphoreTake(_stateMutex, pdMS_TO_TICKS(kMutexTimeoutMs)) == pdTRUE)
        {
            const uint16_t baseline = _sequence.load();
            xSemaphoreGive(_stateMutex);

            subscriber_slot_t& slot = _slots[freeIndex];
            slot.subscriber         = &subscriber;
            slot.filter             = filter;
            slot.active             = true;
            slot.replayPending      = true;
            slot.pendingRemoval     = false;
            slot.baselineSequence   = baseline;
            slot.inFlightCount.store(0);
            result = ERROR_SUCCESS;
        }
        else
        {
            SYS_LOG_E("Notification state lock timed out during subscribe");
            result = ERROR_TIMEOUT;
        }
    }

    xSemaphoreGive(_tableMutex);

    RETURN_IF_ERROR(
        (result != ERROR_SUCCESS),                 // Expression
        result,                                    // Error code
        SYS_LOG_W("Notification subscribe failed") // Error message
    );

    return ERROR_SUCCESS;
}

sys_error_t NotificationManager::unsubscribe(INotificationSubscriber& subscriber)
{
    RETURN_IF_ERROR(
        (_tableMutex == nullptr),                        // Expression
        ERROR_INVALID_STATE,                             // Error code
        SYS_LOG_E("Notification manager is not started") // Error message
    );

    RETURN_IF_ERROR(
        (xTaskGetCurrentTaskHandle() == _dispatcherTask),          // Expression
        ERROR_INVALID_STATE,                                       // Error code
        SYS_LOG_E("unsubscribe() called from the dispatcher task") // Error message
    );

    RETURN_IF_ERROR(
        (xSemaphoreTake(_tableMutex, pdMS_TO_TICKS(kMutexTimeoutMs)) != pdTRUE), // Expression
        ERROR_TIMEOUT,                                                           // Error code
        SYS_LOG_E("Notification table lock timed out")                           // Error message
    );

    /// Stop new deliveries to the slot by marking it for removal.
    size_t      slotIndex  = kMaxSubscribers;
    sys_error_t findResult = _findSlot(subscriber, slotIndex);
    if (findResult == ERROR_SUCCESS)
    {
        _slots[slotIndex].pendingRemoval = true;
    }

    xSemaphoreGive(_tableMutex);

    RETURN_IF_ERROR(
        (findResult != ERROR_SUCCESS),         // Expression
        findResult,                            // Error code
        SYS_LOG_W("Subscriber not registered") // Error message
    );

    /// Wait for in-flight callbacks without holding the table lock.
    const TickType_t startTick = xTaskGetTickCount();
    while ((_slots[slotIndex].inFlightCount.load() != 0) && ((xTaskGetTickCount() - startTick) < pdMS_TO_TICKS(kDrainTimeoutMs)))
    {
        vTaskDelay(1);
    }

    RETURN_IF_ERROR(
        (_slots[slotIndex].inFlightCount.load() != 0),   // Expression
        ERROR_TIMEOUT,                                   // Error code
        SYS_LOG_E("Subscriber callback still in flight") // Error message
    );

    /// Clear the slot only if it is still the pending-removal entry for this subscriber.
    RETURN_IF_ERROR(
        (xSemaphoreTake(_tableMutex, pdMS_TO_TICKS(kMutexTimeoutMs)) != pdTRUE), // Expression
        ERROR_TIMEOUT,                                                           // Error code
        SYS_LOG_E("Notification table lock timed out")                           // Error message
    );

    const bool removable = (_findSlot(subscriber, slotIndex) == ERROR_SUCCESS) && _slots[slotIndex].pendingRemoval;
    if (removable)
    {
        _clearSlot(slotIndex);
    }

    xSemaphoreGive(_tableMutex);

    RETURN_IF_ERROR(
        (!removable),                          // Expression
        ERROR_INVALID_ARG,                     // Error code
        SYS_LOG_W("Subscriber not registered") // Error message
    );

    return ERROR_SUCCESS;
}

/** DISPATCHER ****************************************************************/

void NotificationManager::_dispatcherEntry(void* context)
{
    NotificationManager* manager = static_cast<NotificationManager*>(context);
    manager->_dispatchLoop();
    manager->_dispatcherExited.store(true);
    vTaskSuspend(nullptr);
}

void NotificationManager::_dispatchLoop()
{
    while (!_stopRequested.load())
    {
        if (_pauseRequested.load())
        {
            vTaskDelay(pdMS_TO_TICKS(kDispatchPollMs));
            continue;
        }

        /// Replay retained states to subscribers that registered since the last pass.
        for (size_t i = 0; (i < kMaxSubscribers) && !_stopRequested.load() && !_pauseRequested.load(); ++i)
        {
            _replaySnapshot(i);
        }

        /// Deliver STATE changes whose queue entry was dropped while the queue was full.
        _drainDirtyStates();

        /// Deliver the next queued notification. Stale STATE entries are superseded by newer ones.
        user_notification_t received{};
        if (xQueueReceive(_dispatchQueue, &received, pdMS_TO_TICKS(kDispatchPollMs)) == pdTRUE)
        {
            const bool deliverable = (received.kind == notification_kind_t::EVENT) || _isLatestState(received);
            if (deliverable)
            {
                for (size_t i = 0; i < kMaxSubscribers; ++i)
                {
                    _deliverToSlot(i, received, true);
                }
            }
        }
    }
}

void NotificationManager::_drainDirtyStates()
{
    for (size_t word = 0; word < kDirtyWordCount; ++word)
    {
        if (_stopRequested.load() || _pauseRequested.load())
        {
            return;
        }

        if (xSemaphoreTake(_stateMutex, pdMS_TO_TICKS(kMutexTimeoutMs)) != pdTRUE)
        {
            return;
        }
        uint32_t pending = _dirtyBits[word];
        xSemaphoreGive(_stateMutex);

        for (uint32_t bit = 0; (bit < 32) && (pending != 0); ++bit)
        {
            const uint32_t mask = (1UL << bit);
            if ((pending & mask) == 0)
            {
                continue;
            }
            pending &= ~mask;

            if (_stopRequested.load() || _pauseRequested.load())
            {
                return;
            }

            const size_t        id = (word * 32) + bit;
            user_notification_t notification{};

            /// Clear the dirty bit and copy the entry in one state-lock acquisition.
            if (xSemaphoreTake(_stateMutex, pdMS_TO_TICKS(kMutexTimeoutMs)) != pdTRUE)
            {
                return;
            }
            _dirtyBits[word] &= ~mask;
            notification = _snapshot[id].notification;
            xSemaphoreGive(_stateMutex);

            for (size_t i = 0; i < kMaxSubscribers; ++i)
            {
                _deliverToSlot(i, notification, true);
            }
        }
    }
}

void NotificationManager::_replaySnapshot(size_t slotIndex)
{
    /// Claim the slot. The in-flight count keeps unsubscribe from clearing it mid-replay.
    if (xSemaphoreTake(_tableMutex, pdMS_TO_TICKS(kMutexTimeoutMs)) != pdTRUE)
    {
        return;
    }
    subscriber_slot_t& slot    = _slots[slotIndex];
    const bool         claimed = slot.active && slot.replayPending && !slot.pendingRemoval;
    if (claimed)
    {
        slot.inFlightCount.fetch_add(1);
    }
    xSemaphoreGive(_tableMutex);

    if (!claimed)
    {
        return;
    }

    bool completed = true;
    for (size_t id = 0; id < kNotificationTotalIds; ++id)
    {
        if (_stopRequested.load() || _pauseRequested.load())
        {
            completed = false;
            break;
        }

        /// Copy one entry per state-lock acquisition to keep the task stack small.
        user_notification_t notification{};
        bool                present = false;
        if (xSemaphoreTake(_stateMutex, pdMS_TO_TICKS(kMutexTimeoutMs)) != pdTRUE)
        {
            completed = false;
            break;
        }
        present      = _snapshot[id].present;
        notification = _snapshot[id].notification;
        xSemaphoreGive(_stateMutex);

        if (present)
        {
            _deliverToSlot(slotIndex, notification, false);
        }
    }

    if (completed)
    {
        if (xSemaphoreTake(_tableMutex, pdMS_TO_TICKS(kMutexTimeoutMs)) == pdTRUE)
        {
            slot.replayPending = false;
            xSemaphoreGive(_tableMutex);
        }
        else
        {
            SYS_LOG_W("Notification replay completion not recorded");
        }
    }

    slot.inFlightCount.fetch_sub(1);
}

void NotificationManager::_deliverToSlot(size_t slotIndex, const user_notification_t& notification, bool applyBaseline)
{
    /// Decide eligibility and reserve the slot under the table lock. The callback runs unlocked.
    if (xSemaphoreTake(_tableMutex, pdMS_TO_TICKS(kMutexTimeoutMs)) != pdTRUE)
    {
        SYS_LOG_W("Notification delivery skipped, table lock timed out");
        return;
    }

    subscriber_slot_t&       slot       = _slots[slotIndex];
    INotificationSubscriber* subscriber = slot.subscriber;
    const bool               baselineOk = !applyBaseline || isAfterBaseline(notification.sequence, slot.baselineSequence);
    const bool               eligible   = slot.active && !slot.pendingRemoval && baselineOk && _matchesFilter(slot.filter, notification);
    if (eligible)
    {
        slot.inFlightCount.fetch_add(1);
    }

    xSemaphoreGive(_tableMutex);

    if (!eligible)
    {
        return;
    }

    subscriber->onNotification(notification);
    slot.inFlightCount.fetch_sub(1);
}

bool NotificationManager::_isLatestState(const user_notification_t& notification)
{
    if (xSemaphoreTake(_stateMutex, pdMS_TO_TICKS(kMutexTimeoutMs)) != pdTRUE)
    {
        SYS_LOG_W("Notification state lock timed out, STATE skipped");
        return false;
    }

    const bool latest = (_snapshot[notification.id].notification.sequence == notification.sequence);
    xSemaphoreGive(_stateMutex);
    return latest;
}

/** PRIVATE HELPERS ***********************************************************/

bool NotificationManager::_matchesFilter(const notification_filter_t& filter, const user_notification_t& notification) const
{
    const bool categoryMatch = (filter.categoryMask & static_cast<uint8_t>(notification.category)) != 0;
    const bool severityMatch = static_cast<uint8_t>(notification.severity) >= static_cast<uint8_t>(filter.minSeverity);
    return categoryMatch && severityMatch;
}

sys_error_t NotificationManager::_findSlot(const INotificationSubscriber& subscriber, size_t& slotIndex) const
{
    for (size_t i = 0; i < kMaxSubscribers; ++i)
    {
        if (_slots[i].active && (_slots[i].subscriber == &subscriber))
        {
            slotIndex = i;
            return ERROR_SUCCESS;
        }
    }
    return ERROR_INVALID_ARG;
}

void NotificationManager::_clearSlot(size_t slotIndex)
{
    subscriber_slot_t& slot = _slots[slotIndex];
    slot.subscriber         = nullptr;
    slot.filter             = notification_filter_t{};
    slot.active             = false;
    slot.replayPending      = false;
    slot.pendingRemoval     = false;
    slot.baselineSequence   = 0;
    slot.inFlightCount.store(0);
}

void NotificationManager::_releaseResources()
{
    if (_dispatchQueue != nullptr)
    {
        vQueueDelete(_dispatchQueue);
        _dispatchQueue = nullptr;
    }

    if (_stateMutex != nullptr)
    {
        vSemaphoreDelete(_stateMutex);
        _stateMutex = nullptr;
    }

    if (_tableMutex != nullptr)
    {
        vSemaphoreDelete(_tableMutex);
        _tableMutex = nullptr;
    }
}
