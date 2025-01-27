#pragma once
#include "../state-measure/state.h"


namespace measure {
	using namespace state;

	template<typename T, size_t DIM = 3>
	class Stress : public StateMeasureSchema<T, DIM, 2> {
	public:
		Stress(MaterialPoint<T, DIM>& state, measure::type_schema type_schema, const std::string& name) :
			StateMeasureSchema<T, DIM, 2>(state, name, tens::FILL_TYPE::ZERO, type_schema) {};
	};

	namespace stress {
		const std::string CAUCHY = "S";

		template<typename T>
		class CaushyStress : public StateMeasureSchema<T, 3, 2> {
		public:
			CaushyStress(MaterialPoint<T, 3>& state, measure::type_schema type_schema) : 
				StateMeasureSchema<T, 3, 2>(state, CAUCHY, tens::FILL_TYPE::INDENT, type_schema) {};

			// evolution equation in rate form
			virtual void rate_equation(T t, T dt) override {}

			// evolution equation in finite form
			virtual void finite_equation(T t, T dt) override {};

			virtual T rate_intensity() const override {
				const auto& dS = this->rate();
				return std::sqrt(1.5 * convolution_transp(dS, dS));
			}

			virtual T value_intensity() const override {
				const auto& S = this->value();
				return std::sqrt(1.5 * convolution_transp(S, S));
			}
			template<class T>
			friend std::ostream& operator<<(std::ostream& out, const CaushyStress<T>& m);
		};
		template<class T>
		std::ostream& operator<<(std::ostream& out, const CaushyStress<T>& m) {
			out << m.name() << ": value = " << m.value() << ", rate = " << m.rate();
			return out;
		};
	}
};
