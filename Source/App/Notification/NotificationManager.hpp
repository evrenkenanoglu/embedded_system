/** @file       NotificationManager.hpp
 *  @brief      Publish/subscribe hub for user notifications with a state snapshot.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       01/11/2026
 */

#pragma once

#include "App/Notification/INotificationSubscriber.hpp"
#include "App/Notification/NotificationTypes.hpp"
#include "Process/Process.hpp"

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

#include <atomic>
#include <cstddef>
#include <cstdint>

/**
 * @class NotificationManager
 * @brief Accepts notifications from any producer and delivers them to registered subscribers.
 *
 * Producers call publish() from task context. A dedicated dispatcher task owns all
 * subscriber callbacks. STATE notifications are retained in a snapshot table and are
 * replayed to subscribers that register later. EVENT notifications are delivered once.
 *
 * @note Thread-Safety: publish(), subscribe(), and unsubscribe() may be called from
 *       task context concurrently. Subscriber callbacks run only in the dispatcher task.
 *       Lifecycle methods must be called from one owner task and not from an ISR.
 */
class NotificationManager : public Process
{
public:
    /**
     * @brief Construct the notification hub. Storage is static; no heap is allocated.
     */
    NotificationManager();
    ~NotificationManager() override;

    NotificationManager(const NotificationManager&)            = delete;
    NotificationManager& operator=(const NotificationManager&) = delete;
    NotificationManager(NotificationManager&&)                 = delete;
    NotificationManager& operator=(NotificationManager&&)      = delete;

    sys_error_t start() override;
    sys_error_t stop() override;
    sys_error_t pause() override;
    sys_error_t resume() override;

    /**
     * @brief Publish a notification to the snapshot (STATE) and the dispatch queue.
     * @param[in] notification Id, category, kind, severity, active flag, and payload. sequence is assigned here.
     * @return sys_error_t ERROR_SUCCESS when accepted. A STATE update is never lost; a dropped EVENT is counted and logged.
     * @note Callable from task context only. Must not be called from a subscriber callback.
     */
    sys_error_t publish(const user_notification_t& notification);

    /**
     * @brief Register a subscriber. Active STATE entries matching the filter are replayed to it.
     * @param[in] subscriber Subscriber to register. Must outlive its registration.
     * @param[in] filter     Category mask and minimum severity.
     * @return sys_error_t ERROR_SUCCESS, ERROR_INVALID_ARG if already registered or null, or ERROR_OUT_OF_MEMORY if the table is full.
     */
    sys_error_t subscribe(INotificationSubscriber& subscriber, notification_filter_t filter);

    /**
     * @brief Remove a subscriber. Returns only after any in-flight callback for it has finished.
     * @param[in] subscriber Subscriber previously registered with subscribe().
     * @return sys_error_t ERROR_SUCCESS, ERROR_INVALID_ARG if not registered, or ERROR_TIMEOUT if a callback did not finish in time.
     * @warning Must not be called from within a subscriber callback.
     */
    sys_error_t unsubscribe(INotificationSubscriber& subscriber);

private:
    static constexpr size_t   kMaxSubscribers       = 8;
    static constexpr size_t   kDispatchQueueLength  = 32;
    static constexpr uint32_t kMutexTimeoutMs       = 100;
    static constexpr uint32_t kDrainTimeoutMs       = 100;
    static constexpr uint32_t kDispatchPollMs       = 50;
    static constexpr uint32_t kDispatcherStackBytes = 3072;
    static constexpr uint8_t  kDispatcherPriority   = 2;

    struct snapshot_entry_t
    {
        bool                present;
        user_notification_t notification;
    };

    struct subscriber_slot_t
    {
        INotificationSubscriber* subscriber;
        notification_filter_t    filter;
        bool                     active;
        bool                     replayPending;
        bool                     pendingRemoval;
        uint16_t                 baselineSequence;
        std::atomic<uint32_t>    inFlightCount;
    };

    static void _dispatcherEntry(void* context);
    void        _dispatchLoop();
    void        _drainDirtyStates();
    void        _replaySnapshot(size_t slotIndex);
    void        _deliverToSlot(size_t slotIndex, const user_notification_t& notification, bool applyBaseline);
    bool        _isLatestState(const user_notification_t& notification);
    bool        _matchesFilter(const notification_filter_t& filter, const user_notification_t& notification) const;
    sys_error_t _findSlot(const INotificationSubscriber& subscriber, size_t& slotIndex) const;
    void        _clearSlot(size_t slotIndex);
    void        _releaseResources();

    TaskHandle_t      _dispatcherTask;
    QueueHandle_t     _dispatchQueue;
    SemaphoreHandle_t _stateMutex;
    SemaphoreHandle_t _tableMutex;

    snapshot_entry_t  _snapshot[kNotificationTotalIds];
    uint32_t          _dirtyBits[kNotificationTotalIds / 32];
    subscriber_slot_t _slots[kMaxSubscribers];

    std::atomic<uint16_t> _sequence;
    std::atomic<uint32_t> _droppedEvents;
    std::atomic<bool>     _stopRequested;
    std::atomic<bool>     _pauseRequested;
    std::atomic<bool>     _dispatcherExited;
};
