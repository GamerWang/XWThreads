//-------------------------------------------------------------------------------
//-------------------------------------------------------------------------------

#ifndef _XW_FUNCTION_H_INCLUDED_
#define _XW_FUNCTION_H_INCLUDED_

//-------------------------------------------------------------------------------

#include <memory>
#include <cassert>

//-------------------------------------------------------------------------------
namespace XW {
//-------------------------------------------------------------------------------

template <typename T>
class Function;

template <typename ReturnValue, typename... Args>
class Function<ReturnValue(Args...)> {
public:
	template <typename T>
	Function& operator=(T t) {
		callable_ = std::make_unique<CallableT<T>>(t);
		return *this;
	}

	ReturnValue operator()(Args... args) {
		assert(callable_);
		return callable_->Invoke(args...);
	}
private:
	class ICallable {
	public: 
		virtual ~ICallable() = default;
		virtual ReturnValue Invoke(Args...) = 0;
	};

	template<typename T>
	class CallableT : public ICallable {
	public:
		CallableT(const T& t) : t_(t) {
		}

		~CallableT() override = default;

		ReturnValue Invoke(Args... args) override {
			return t_(args...);
		}
	private:
		T t_;
	};

	std::unique_ptr<ICallable> callable_;
};

//-------------------------------------------------------------------------------
} // namespace XW
//-------------------------------------------------------------------------------
#endif