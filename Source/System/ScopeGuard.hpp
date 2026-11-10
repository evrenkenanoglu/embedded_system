/** @file       ScopeGuard.hpp
 *  @brief      Generic scope-exit cleanup guard.
 *  @copyright  (c) 2026- Evren Kenanoglu - All Rights Reserved
 *              Permission to use, reproduce, copy, prepare derivative works,
 *              modify, distribute, perform, display or sell this software and/or
 *              its documentation for any purpose is prohibited without the express
 *              written consent of Evren Kenanoglu.
 *  @author     Evren Kenanoglu
 *  @date       2026
 */

#pragma once

#include <utility>

/**
 * @class ScopeGuard
 * @brief Runs a cleanup callable exactly once on scope exit unless dismiss() was called.
 *
 * @note The callable must not throw; it must report failures through logging.
 *       The guard is neither copyable nor movable, so the callable cannot run twice.
 */
template <typename Fn>
class ScopeGuard
{
public:
    explicit ScopeGuard(Fn&& cleanup)
        : _cleanup(std::move(cleanup))
        , _armed(true)
    {
    }

    ScopeGuard(const ScopeGuard&)            = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;
    ScopeGuard(ScopeGuard&&)                 = delete;
    ScopeGuard& operator=(ScopeGuard&&)      = delete;

    ~ScopeGuard()
    {
        if (_armed)
        {
            _cleanup();
        }
    }

    void dismiss() noexcept
    {
        _armed = false;
    }

private:
    Fn   _cleanup;
    bool _armed;
};
