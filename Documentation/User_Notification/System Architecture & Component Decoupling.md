Here are the Mermaid diagrams representing the **User Notification Manager**:

1. **System Architecture & Data Flow Diagram** (Structure, thread boundaries, and hardware single-writer isolation).
2. **Lifecycle & Thread-Safe Synchronization Sequence Diagram** (Publishing, asynchronous replay on subscription, and deadlock-free unsubscription).

---

### 1. System Architecture & Component Decoupling

```mermaid
flowchart TB
    %% =========================================================================
    %% PRODUCERS
    %% =========================================================================
    subgraph SUB_PRODUCERS ["1. Producers (Publishers)"]
        direction TB
        subgraph FRAMEWORK_PRODUCERS ["Framework Domain (IDs: 0x00..0x7F)"]
            P_WIFI["WifiConfigurationManager<br/>(Bridge Adapter)"]
            P_MATTER["MatterWrapper<br/>(Stack Events)"]
            P_OTA["OtaService<br/>(Progress / State)"]
            P_DIAG["Silicon Health / WDT<br/>(System Faults)"]
        end

        subgraph APP_PRODUCERS ["Application Domain (IDs: 0x80..0xFF)"]
            P_RELAY["Proc_PowerSwitch<br/>(Relay State)"]
            P_TEMP["NTC Thermal Sensor<br/>(Trip / Release)"]
        end
    end

    %% =========================================================================
    %% NOTIFICATION HUB
    %% =========================================================================
    subgraph SUB_HUB ["2. NotificationManager Engine (App Layer)"]
        direction TB

        API_PUB["publish(id, category, kind, severity, active, payload)"]
        API_SUB["subscribe(subscriber, filter)"]
        API_UNSUB["unsubscribe(subscriber)"]

        subgraph STORAGE ["Thread-Safe Core Storage"]
            SNAPSHOT[("State Snapshot Table [256]<br/>(guarded by _stateMutex)")]
            DIRTY_MASK["Coalescing Dirty Bitmask<br/>(Eventual Consistency Fallback)"]
            REGISTRY[("Subscriber Registry [8 Slots]<br/>- INotificationSubscriber*<br/>- notification_filter_t<br/>- inFlightCount & pendingRemoval<br/>(guarded by _tableMutex)")]
        end

        QUEUE[["Unified Dispatch FIFO Queue<br/>- user_notification_t<br/>- MSG_CTRL_REPLAY"]]

        TASK_DISPATCH{{"FreeRTOS Dispatcher Task<br/>(No locks held during callbacks)"}}
    end

    %% =========================================================================
    %% SUBSCRIBERS & SINKS
    %% =========================================================================
    subgraph SUB_SUBSCRIBERS ["3. Subscribers & Hardware Output Isolation"]
        direction TB
        IF_SUB["<< interface >><br/>INotificationSubscriber<br/>onNotification(user_notification_t)"]

        S_LED["StatusLedNotificationSubscriber<br/>(Filters: OTA, Connect, App)"]
        S_LOG["SerialNotificationSubscriber<br/>(Filters: minSeverity >= WARNING)"]
        S_HTTP["HttpNotificationSubscriber<br/>(Phase 2 - Filter: ALL)"]

        PROC_LED["Proc_StatusIndicator<br/>(Dedicated Worker Task)"]
        SINK_GPIO["io_gpio (Status LED)<br/>★ SINGLE-WRITER ISOLATION"]
        SINK_LOG["SYS_LOG_* / Console<br/>(Zero Secret Leaks)"]
        SINK_WS["WebSocket / Browser UI<br/>(IWebSocketUri)"]
    end

    %% =========================================================================
    %% CONNECTIONS
    %% =========================================================================
    %% Producer calls
    P_WIFI -->|"publish(WIFI_*)"| API_PUB
    P_MATTER -->|"publish(MATTER_*)"| API_PUB
    P_OTA -->|"publish(OTA_*)"| API_PUB
    P_DIAG -->|"publish(SYS_* / DIAG_*)"| API_PUB
    P_RELAY -->|"publish(APP_NOTIF_RELAY_*)"| API_PUB
    P_TEMP -->|"publish(DIAG_THERMAL_*)"| API_PUB

    %% Internal Hub Mechanics
    API_PUB -->|"Update STATE under lock"| SNAPSHOT
    API_PUB -->|"Enqueue message"| QUEUE
    API_PUB -.->|"Queue full fallback"| DIRTY_MASK
    DIRTY_MASK -.->|"Drain on space available"| TASK_DISPATCH

    API_SUB -->|"Register slot"| REGISTRY
    API_SUB -->|"Enqueue MSG_CTRL_REPLAY"| QUEUE

    API_UNSUB -->|"Mark pendingRemoval"| REGISTRY

    QUEUE -->|"Pop event / replay"| TASK_DISPATCH
    SNAPSHOT -.->|"Copy active states for replay"| TASK_DISPATCH
    REGISTRY -.->|"Check filter & inFlightCount"| TASK_DISPATCH

    %% Subscriber Dispatch
    TASK_DISPATCH ==>|"Dispatch (non-blocking)"| IF_SUB

    IF_SUB --> S_LED
    IF_SUB --> S_LOG
    IF_SUB --> S_HTTP

    %% Outputs
    S_LED -->|"setState(indication_state_t)"| PROC_LED
    PROC_LED -->|"Hardware GPIO Output"| SINK_GPIO
    S_LOG -->|"Structured Log Output"| SINK_LOG
    S_HTTP -->|"JSON Frame Broadcast"| SINK_WS

    %% Styling
    classDef producer fill:#e1f5fe,stroke:#0288d1,stroke-width:1px,color:#01579b;
    classDef hub fill:#fff3e0,stroke:#f57c00,stroke-width:2px,color:#e65100;
    classDef task fill:#ffe0b2,stroke:#e65100,stroke-width:2px,color:#bf360c;
    classDef subscriber fill:#e8f5e9,stroke:#388e3c,stroke-width:1px,color:#1b5e20;
    classDef hardware fill:#fce4ec,stroke:#c2185b,stroke-width:2px,color:#880e4f;

    class P_WIFI,P_MATTER,P_OTA,P_DIAG,P_RELAY,P_TEMP producer;
    class API_PUB,API_SUB,API_UNSUB,SNAPSHOT,DIRTY_MASK,REGISTRY,QUEUE hub;
    class TASK_DISPATCH task;
    class IF_SUB,S_LED,S_LOG,S_HTTP,SINK_LOG,SINK_WS subscriber;
    class PROC_LED,SINK_GPIO hardware;
```

---

### 2. Runtime Flow: Asynchronous Replay & Deadlock-Free Drain Sequence

```mermaid
sequenceDiagram
    autonumber
    participant AppTask as Caller / Application Task
    participant Hub as NotificationManager Core
    participant Queue as Dispatcher FIFO Queue
    participant DispTask as Dispatcher Task (Worker)
    participant Sub as INotificationSubscriber

    %% =========================================================================
    %% SUBSCRIBE & ASYNC REPLAY
    %% =========================================================================
    rect rgb(6, 131, 240)
        Note over AppTask, Sub: Phase A: Subscription with Asynchronous Replay
        AppTask->>Hub: subscribe(subscriber, filter)
        activate Hub
        Hub->>Hub: Take _tableMutex (bounded timeout)
        Hub->>Hub: Allocate slot in Registry (active=true, pendingRemoval=false)
        Hub->>Queue: Push CMD_REPLAY { slotIndex }
        Hub->>Hub: Release _tableMutex
        Hub-->>AppTask: return ERROR_SUCCESS
        deactivate Hub

        Note over Queue, DispTask: Dispatcher processes Replay in its own task context
        Queue->>DispTask: Pop CMD_REPLAY { slotIndex }
        activate DispTask
        DispTask->>Hub: Take _stateMutex -> Copy active snapshot states matching filter
        Hub-->>DispTask: Copied matching active STATEs
        DispTask->>Hub: Release _stateMutex (No lock held during callback!)
        loop For each matching active state
            DispTask->>Sub: onNotification(replayedState)
        end
        deactivate DispTask
    end

    %% =========================================================================
    %% ASYNC LIVE PUBLISH
    %% =========================================================================
    rect rgb(228, 154, 6)
        Note over AppTask, Sub: Phase B: Live Notification Publishing
        AppTask->>Hub: publish(OTA_UPDATING, STATE, active=true)
        activate Hub
        Hub->>Hub: Take _stateMutex -> Update _stateSnapshot[OTA_UPDATING]
        Hub->>Hub: Release _stateMutex
        Hub->>Queue: Push user_notification_t
        Hub-->>AppTask: return ERROR_SUCCESS
        deactivate Hub

        Queue->>DispTask: Pop user_notification_t
        activate DispTask
        DispTask->>Hub: Take _tableMutex -> Check filter & increment inFlightCount
        DispTask->>Hub: Release _tableMutex
        DispTask->>Sub: onNotification(liveNotification)
        DispTask->>Hub: Take _tableMutex -> Decrement inFlightCount
        DispTask->>Hub: Release _tableMutex
        deactivate DispTask
    end

    %% =========================================================================
    %% DEADLOCK-FREE UNSUBSCRIBE
    %% =========================================================================
    rect rgb(178, 35, 83)
        Note over AppTask, DispTask: Phase C: Deadlock-Free Safe Unsubscription
        AppTask->>Hub: unsubscribe(subscriber)
        activate Hub
        Hub->>Hub: Take _tableMutex
        Hub->>Hub: Set slot.pendingRemoval = true
        Hub->>Hub: Release _tableMutex

        alt slot.inFlightCount == 0 (Callback not in progress)
            Hub->>Hub: Take _tableMutex -> Clear slot -> Release _tableMutex
            Hub-->>AppTask: return ERROR_SUCCESS
        else slot.inFlightCount > 0 (Callback currently executing in DispTask)
            Note over Hub, DispTask: Wait for in-flight execution to drain safely
            Hub->>Hub: Wait on _drainSemaphore (timeout: 100ms)
            DispTask->>Sub: (Finishes executing onNotification)
            DispTask->>Hub: inFlightCount becomes 0 -> Gives _drainSemaphore
            Hub->>Hub: Take _tableMutex -> Clear slot -> Release _tableMutex
            Hub-->>AppTask: return ERROR_SUCCESS
        end
        deactivate Hub
        Note over AppTask: Safe to destroy subscriber object (No dangling pointer)
    end
```
