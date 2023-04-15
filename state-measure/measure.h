#pragma once

//#include "schema.h"
namespace state {
	template<typename T>
	class State;
};

namespace measure {
	using namespace state;

	namespace error {
		class StateNotLinked : public std::exception {
		public:
			virtual const char* what() const noexcept {
				return "Measure::state not linked or alredy has been destroyed";
			};
		};
	}

	template<template<class> class Q, class T>
	class AbstractMeasure {
		std::string _name;
		Q<T>& _value;
		Q<T> _rate;
		Q<T> _value_prev;
		Q<T> _rate_prev;
		int _lock = 0; // to prevent updates during calc step
	protected:
		// use reference rate_temp / value_temp for temporary calculation to prevent a new allocation
		// update_rate()/update_value() use rate_temp / value_temp value as a new one (for more info see declaration)
		// WARNING: rate_temp / value_temp variables updating and calling update_rate()/update_value() must be in one scope
		Q<T> rate_temp;
		Q<T> value_temp;
	public:
		AbstractMeasure(std::string name, Q<T>& value, Q<T>&& rate) :
			_name(name),
			_value(value),
			_rate(std::move(rate)),
			_value_prev(_value),
			_rate_prev(_rate),
			value_temp(_value),
			rate_temp(_rate) {};

		// const refs to private fields
		const Q<T>& rate() const { return _rate; };
		const Q<T>& value() const { return _value; };
		const Q<T>& rate_prev() const { return _rate_prev; };
		const Q<T>& value_prev() const { return _value_prev; };
		const std::string& name() const { return _name; };

		// to prevent modifying rate and value you should lock measure
		int lock() {
			return _lock = rand();
		}
		// to modify rate and value you should unlock measure
		bool unlock(int lock_value) {
			if (lock_value == _lock) {
				_lock = 0;
				return true;
			}
			return false;
		}
		// method uses value_temp as a new one (swap pointers, more efficient)
		void update_value() {
#ifdef _DEBUG
			if (_lock) throw new std::logic_error("updates are locked");
#endif
			// _value = _value_temp, _value_temp = _value
			std::swap(_value, value_temp);
			// _value_temp = _value_prev, _value_prev = _value_temp
			std::swap(_value_prev, value_temp);
			// _value = _value_temp, _value_prev = _value, _value_temp = _value_prev
		};

		// method uses rate_temp as a new one (swap pointers, more efficient)
		void update_rate() {
#ifdef _DEBUG
			if (_lock) throw new std::logic_error("updates are locked");
#endif
			std::swap(_rate, rate_temp);
			std::swap(_rate_prev, rate_temp);
		};

		// ============================================================================= //
		//		PERFORMANCE NOTE: to prevent unnecessery allocations use				 //
		//		[+=, -=, /= , *=] methods instead of [+, -, /, *] when possible			 //
		// ============================================================================= //

		// evolution equation in rate form
		// rate must be updated by calling update_rate
		// use reference this->rate_temp() for temporary calculation to prevent a new allocation
		// update_rate() uses this->rate_temp value as a new one (for more info see declaration)
		virtual void rate_equation() = 0;

		// evolution equation in finite form
		// value must be updated by calling update_value
		// use reference this->value_temp() for temporary calculation to prevent a new allocation
		// update_value() uses this->value_temp value as a new one (for more info see declaration)
		virtual void finit_equation() = 0;

		// the first order schema to calculate rate, may be overriden
		// use reference this->rate_temp() for temporary calculation to prevent a new allocation
		// update_rate() uses this->rate_temp value as a new one (for more info see declaration)
		virtual void calc_rate(T dt) {
			rate_temp = _value;
			rate_temp -= _value_prev;
			rate_temp /= dt;
		};

		// the first order (Euler) schema to integrate value, may be overriden
		// use this->value_temp() for temporary calculation to prevent a new allocation
		// update_value() uses this->value_temp value as a new one (for more info see declaration)
		virtual void integrate_value(T dt) {
			value_temp = _rate;
			value_temp *= dt;
			value_temp += _value;
		};

		template<template<class> class Q, class T>
		friend std::ostream& operator<<(std::ostream& out, const AbstractMeasure<Q, T>& m);
	};

	template<template<class> class Q, class T>
	std::ostream& operator<<(std::ostream& out, const AbstractMeasure<Q, T>& m) {
		out << m._name << ": value = " << m._value << ", rate = " << m._rate;
		return out;
	};

	template<typename T>
	class StateMeasure : public tens::object<T>, public AbstractMeasure<tens::container, T> {
		std::weak_ptr<State<T>> _state;
	public:
		StateMeasure(std::shared_ptr<State<T>>& state, size_t dim, size_t rank, std::string name, tens::FILL_TYPE type = tens::FILL_TYPE::ZERO) :
			tens::object<T>(dim, rank, type, state->basis()),
			_state(state),
			AbstractMeasure<tens::container, T>(
				name,
				this->comp(), // link ref
				tens::container<T>(dim, rank, tens::FILL_TYPE::ZERO)) {
		};

		// TODO: looks like a bit weird -> fix
		// becasue StateMeasure is AbstractMeasure
		StateMeasure(StateMeasure&& measure) noexcept : 
			tens::object<T>(std::move(measure)),
			AbstractMeasure<tens::container, T>(
				measure.name(),
				this->comp(),
				tens::container<T>(std::move(measure.rate()))),
			_state(measure._state){
		}

		// access by const ref to other Measures in the State
		const StateMeasure<T>& operator[] (std::string name) {
			if (std::shared_ptr<State<T>> state = this->_state.lock()) {
				return *(*state.get())[name];
			}
			throw new error::StateNotLinked();
		}
	};
};
