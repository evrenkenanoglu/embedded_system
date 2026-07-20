# AI Code Review Report

## 1. Summary of Changes
[Provide a brief 2-3 sentence high-level overview of the reviewed changes, focusing on FreeRTOS tasks, synchronization primitives, C++ object lifecycles, or hardware interactions.]

## 2. Major Critical Issues
| File        | Line     | Issue Description                                                                                   | Proposed Fix / Recommendation                                             |
| :---------- | :------- | :-------------------------------------------------------------------------------------------------- | :------------------------------------------------------------------------ |
| [File Name] | [Line #] | [Describe major bug, memory leak, UB, ISR blocking, or incorrect C++ FreeRTOS task entry signature] | [Provide a clean, thread-safe C++ code fix using FreeRTOS best practices] |

## 3. General & Code Quality Suggestions
* **[File Name]:** [Bullet point recommendation for modern C++ features, static vs dynamic allocation, code readability, or styling.]

## 4. Security & Embedded Safety Impact
[Discuss if the changes present any stack/heap overflow hazards, race conditions, priority inversions, deadlocks, or hardware-level safety exploits.]