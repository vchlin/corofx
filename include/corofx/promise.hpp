#pragma once

#include "corofx/check.hpp"
#include "corofx/effect.hpp"
#include "detail/type_id.hpp"

#include <coroutine>
#include <optional>
#include <tuple>
#include <utility>

namespace corofx {

struct unit {};

class promise_base;

template<typename T = void>
class promise_impl;

class handler_list;

class resumer_tag {
public:
    resumer_tag(resumer_tag const&) = delete;
    resumer_tag(resumer_tag&&) = delete;
    ~resumer_tag() = default;
    auto operator=(resumer_tag const&) -> resumer_tag& = delete;
    auto operator=(resumer_tag&&) -> resumer_tag& = delete;

private:
    template<effect E>
    friend class resumer;
    friend promise_base;

    explicit resumer_tag(promise_base& resume) noexcept : resume_{resume} {}

    promise_base& resume_;
};

template<effect E>
class effect_awaiter;

template<effect E>
class resumer {
public:
    resumer(resumer const&) = delete;
    resumer(resumer&&) = delete;
    ~resumer() = default;
    auto operator=(resumer const&) -> resumer& = delete;
    auto operator=(resumer&&) -> resumer& = delete;

    // TODO: Handle void return_type.
    auto operator()(E::return_type value) noexcept -> resumer_tag {
        effect_.set_value(std::move(value));
        return resumer_tag{resume_};
    }

private:
    friend class effect_awaiter<E>;

    explicit resumer(promise_base& resume, effect_awaiter<E>& effect) noexcept
        : resume_{resume}, effect_{effect} {}

    promise_base& resume_;
    effect_awaiter<E>& effect_;
};

class untyped_task {
public:
    untyped_task() noexcept = default;
    explicit untyped_task(promise_base* p) noexcept : promise_{p} {}
    untyped_task(untyped_task const&) = delete;
    untyped_task(untyped_task&& that) noexcept : promise_{std::exchange(that.promise_, {})} {}

    ~untyped_task();

    auto operator=(untyped_task const&) -> untyped_task& = delete;

    auto operator=(untyped_task&& that) noexcept -> untyped_task& {
        untyped_task{std::move(that)}.swap(*this);
        return *this;
    }

    explicit operator bool() const noexcept { return promise_ != nullptr; }

private:
    template<effect E>
    friend class effect_awaiter;
    friend promise_base;

    auto swap(untyped_task& that) noexcept -> void {
        using std::swap;
        swap(promise_, that.promise_);
    }

    promise_base* promise_{};
};

// Base promise type.
class promise_base {
public:
    struct final_awaiter : std::suspend_always {
        template<std::derived_from<promise_base> U>
        auto await_suspend(std::coroutine_handle<U> frame) noexcept -> std::coroutine_handle<> {
            using base_ptr = promise_base*;
            auto* p = base_ptr{&frame.promise()};
            if (p->handling()) p = p->cont_;
            if (auto* k = p->cont_) return k->frame_;
            return std::noop_coroutine();
        }
    };

    template<typename P>
    class task_awaiter;
    template<typename HT>
    class handled_task_awaiter;

    promise_base(promise_base const&) = delete;
    promise_base(promise_base&&) = delete;
    auto operator=(promise_base const&) -> promise_base& = delete;
    auto operator=(promise_base&&) -> promise_base& = delete;

    [[nodiscard]]
    constexpr auto initial_suspend() const noexcept -> std::suspend_always {
        return {};
    }

    auto return_value(resumer_tag const& resume) noexcept -> void {
        set_cont(resume.resume_);
        set_handling(false);
    }

    [[noreturn]] auto unhandled_exception() noexcept -> void { unreachable("unhandled exception"); }
    [[nodiscard]] auto final_suspend() const noexcept -> final_awaiter { return {}; }

    template<effect E>
    auto create_handler(E& eff, resumer<E>& resumer) noexcept -> untyped_task;

    [[nodiscard]] auto get_frame() const noexcept -> std::coroutine_handle<> { return frame_; }
    auto set_handling(bool handling) noexcept -> void { is_handler_ = handling; }
    auto set_handlers(handler_list& handlers) noexcept -> void { handlers_ = &handlers; }

protected:
    promise_base(std::coroutine_handle<> frame) noexcept : frame_{frame} {}
    ~promise_base() = default;

    [[nodiscard]] auto handling() const noexcept -> bool { return is_handler_; }
    [[nodiscard]] auto get_cont() const noexcept -> promise_base& { return *cont_; }

private:
    auto set_cont(promise_base& cont) noexcept -> std::coroutine_handle<> {
        cont_ = &cont;
        return frame_;
    }

    std::coroutine_handle<> const frame_;
    promise_base* cont_{};
    handler_list* handlers_{};
    bool is_handler_{};
};

// A list of effect handlers.
class handler_list {
public:
    handler_list(handler_list const&) = default;
    handler_list(handler_list&&) = default;
    auto operator=(handler_list const&) -> handler_list& = default;
    auto operator=(handler_list&&) -> handler_list& = default;

    template<effect E>
    auto handle(E&& eff, resumer<E>& resume) noexcept -> untyped_task {
        return handle_untyped(type_id_for<E>, &eff, &resume);
    }

protected:
    handler_list() noexcept = default;
    ~handler_list() = default;

private:
    virtual auto handle_untyped(type_id id, void* eff, void* resume) noexcept -> untyped_task = 0;
};

template<typename P>
class promise_base::task_awaiter : public std::suspend_always {
public:
    explicit task_awaiter(P& p) noexcept : p_{p} {}
    task_awaiter(task_awaiter const&) = delete;
    task_awaiter(task_awaiter&&) = delete;
    ~task_awaiter() = default;
    auto operator=(task_awaiter const&) -> task_awaiter& = delete;
    auto operator=(task_awaiter&&) -> task_awaiter& = delete;

    template<std::derived_from<promise_base> U>
    auto await_suspend(std::coroutine_handle<U> frame) noexcept -> std::coroutine_handle<> {
        return p_.set_cont(frame.promise());
    }

    auto await_resume() noexcept -> P::value_type { return p_.take_value(); }

private:
    P& p_;
};

template<typename HT>
class promise_base::handled_task_awaiter : public std::suspend_always {
public:
    explicit handled_task_awaiter(HT ht) noexcept : ht_{std::move(ht)} {}

    template<std::derived_from<promise_base> U>
    auto await_suspend(std::coroutine_handle<U> frame) noexcept -> std::coroutine_handle<> {
        return ht_.task_.frame_.promise().set_cont(frame.promise());
    }

    auto await_resume() noexcept -> HT::task_type::promise_type::value_type {
        return ht_.task_.frame_.promise().take_value();
    }

private:
    HT ht_;
};

template<effect E>
class effect_awaiter : public std::suspend_always {
public:
    using value_type = E::return_type;

    explicit effect_awaiter(promise_base* p, E eff) noexcept
        : eff_{std::move(eff)}, resumer_{*p, *this}, task_(p->create_handler(eff_, resumer_)) {}

    template<std::derived_from<promise_base> U>
    auto await_suspend(std::coroutine_handle<U>) noexcept -> std::coroutine_handle<> {
        return task_.promise_->get_frame();
    }

    auto await_resume() noexcept -> value_type { return std::move(*value_); }

    auto set_value(value_type value) noexcept -> void { value_ = std::move(value); }

private:
    E eff_; // NOTE: This effect will not be moved until the task starts running.
    resumer<E> resumer_;
    untyped_task task_;
    std::optional<value_type> value_;
};

template<>
class promise_impl<> : public promise_base {
public:
    using promise_base::promise_base;
    using promise_base::return_value;

    constexpr auto return_value(unit) noexcept -> void {}

    constexpr auto take_value() const noexcept -> void {}
};

template<typename T>
class promise_impl : public promise_base {
public:
    using promise_base::promise_base;
    using promise_base::return_value;

    auto return_value(T value) noexcept -> void {
        if (handling()) {
            // SAFETY: A handler returns the same type as the function it runs with. This is
            // checked at compile time in `task::with`.
            auto& k = static_cast<promise_impl<T>&>(get_cont());
            k.return_value(std::move(value));
        } else {
            value_ = std::move(value);
        }
    }

    auto take_value() noexcept -> T { return std::move(*value_); }

private:
    std::optional<T> value_;
};

// An effect handler entry.
template<effect E, typename F>
class handler {
public:
    using task_type = std::invoke_result_t<F, E&&, resumer<E>&>;
    using effect_type = E;

    handler(F fn) noexcept : fn_{std::move(fn)} {}

    auto handle(type_id id, void* eff, void* resume) noexcept -> untyped_task {
        if (id != type_id_for<E>) return {};
        // SAFETY: Type ID is checked above to ensure we are handling the right effect.
        auto task = fn_(std::move(*static_cast<E*>(eff)), *static_cast<resumer<E>*>(resume));
        auto& p = task.release().promise();
        p.set_handling(true);
        return untyped_task{&p};
    }

private:
    F fn_;
};

// Creates an effect handler entry.
template<effect E, typename F>
auto make_handler(F fn) noexcept -> handler<E, F> {
    return handler<E, F>{std::move(fn)};
}

template<typename... Hs>
class handler_list_impl : public handler_list {
public:
    handler_list_impl(Hs... handlers) noexcept
        : handlers_{std::make_tuple(std::move(handlers)...)} {}

private:
    auto handle_untyped(type_id id, void* eff, void* resume) noexcept -> untyped_task override {
        return std::apply(
            [=](auto&... hs) {
                auto h = untyped_task{};
                ((h = hs.handle(id, eff, resume)) || ...);
                return h;
            },
            handlers_);
    }

    std::tuple<Hs...> handlers_{};
};

inline untyped_task::~untyped_task() {
    if (promise_) promise_->get_frame().destroy();
}

template<effect E>
auto promise_base::create_handler(E& eff, resumer<E>& resumer) noexcept -> untyped_task {
    auto* p = this;
    if (p->handling()) p = p->cont_->cont_;
    for (; p; p = p->cont_) {
        if (auto* hs = p->handlers_) {
            if (auto h = hs->handle(std::move(eff), resumer)) {
                h.promise_->set_cont(*p);
                return std::move(h);
            }
        }
    }
    unreachable("unhandled effect");
}

} // namespace corofx
