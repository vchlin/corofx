#pragma once

#include "effect.hpp"
#include "frame.hpp"

#include <optional>

namespace corofx {

template<effect E>
class evidence {
protected:
    auto set_handler(handler<E>* h) noexcept -> void { handler_ = h; }

    [[nodiscard]]
    auto get_handler() const noexcept -> handler<E>* {
        return handler_;
    }

private:
    handler<E>* handler_{};
};

template<effect... Es>
class evidence_vec : public evidence<Es>... {
public:
    using evidence<Es>::set_handler...;

    template<effect E>
    [[nodiscard]]
    auto get_handler() const noexcept -> handler<E>* {
        return evidence<E>::get_handler();
    }
};

// An effect handler entry.
template<effect E, typename F>
class handler_impl : public handler<E> {
public:
    using effect_type = E;
    using task_type = std::invoke_result_t<F, E&&, resumer<E>&>;
    using value_type = task_type::value_type;

    handler_impl(F fn) noexcept : fn_{std::move(fn)} {}

    [[nodiscard]]
    auto handle(E&& eff, resumer<E>& resume) noexcept -> frame<> final {
        auto task = fn_(std::move(eff), resume);
        auto& p = task.frame_->promise();
        p.set_cont(cont_);
        p.set_output(*output_);
        task_type::effect_types::apply(
            [&]<effect... Es>() { (p.set_handler(ev_vec_.template get_handler<Es>()), ...); });
        return std::move(task);
    }

    template<typename Task>
    auto copy_handlers(Task& t) noexcept -> void {
        if constexpr (not task_type::effect_types::empty) {
            task_type::effect_types::apply(
                [&]<effect... Es>() { (ev_vec_.set_handler(t.template get_handler<Es>()), ...); });
        }
    }

    auto set_cont(std::coroutine_handle<> cont) noexcept -> void { cont_ = cont; }
    auto set_output(std::optional<value_holder<value_type>>& output) noexcept -> void {
        output_ = &output;
    }

private:
    F fn_;
    std::coroutine_handle<> cont_;
    std::optional<value_holder<value_type>>* output_{};
    task_type::effect_types::template unpack_to<evidence_vec> ev_vec_;
};

// Creates an effect handler entry.
template<effect E, typename F>
[[nodiscard]]
auto handler_of(F fn) noexcept -> handler_impl<E, F> {
    return handler_impl<E, F>{std::move(fn)};
}

} // namespace corofx
