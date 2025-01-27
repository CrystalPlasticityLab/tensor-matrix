#pragma once
#include <algorithm>
#include <array>
#include <cassert>
#include <concepts>
#include <iostream>
#include <memory>
#include <random>
#include <utility>
#include "error.h"
#include "math.h"
#include <Eigen/Dense>
#include <Eigen/Eigenvalues>

extern std::mt19937 gen;       // Standard mersenne_twister_engine seeded with rd()
extern std::uniform_real_distribution<double> unidistr;

namespace tens {

	constexpr size_t pow(size_t base, size_t exp) {
		return exp == 0 ? 1 : base * pow(base, exp - 1);
	}

	template<typename T, size_t DIM, size_t RANK>
	concept FloatPoint = std::floating_point<T>;

	enum class FILL_TYPE {
		ZERO,
		RANDOM,
		RANDOMUNIT,
		RANDOMSYMM,
		INDENT
	};

	template<typename T, size_t DIM, size_t RANK>
	requires FloatPoint<T, DIM, RANK>
	class container;
	template<typename T, size_t DIM, size_t RANK> 
	std::ostream& operator<< (std::ostream& o, const container<T, DIM, RANK>& cont);

	template<typename T, size_t DIM, size_t RANK = 2>
	container<T, DIM, RANK> Matrix(const std::array<std::array<T, DIM>, DIM>& matrix) {
		std::array<T, pow(DIM, RANK)> arr;
		if (DIM == 3) {// ------- for DIM == 3 : {00, 11, 22, 12, 02, 01, 21, 20, 10} 
			arr[0] = matrix[0][0];
			arr[1] = matrix[1][1];
			arr[2] = matrix[2][2];
			arr[3] = matrix[1][2];
			arr[4] = matrix[0][2];
			arr[5] = matrix[0][1];
			arr[6] = matrix[2][1];
			arr[7] = matrix[2][0];
			arr[8] = matrix[1][0];
			return container<T, DIM, RANK>(arr);
		}
		throw new NoImplemetationYet();
	}
	
	template<typename T, size_t DIM, size_t RANK = 2>
	container<T, DIM, RANK> Matrix(FILL_TYPE type) {
		return container<T, DIM, RANK>(type);
	}

	template<typename T, size_t DIM, size_t RANK = 1>
	container<T, DIM, RANK> Array(FILL_TYPE type) {
		return container<T, DIM, RANK>(type);
	}

	template<typename T, size_t DIM, size_t RANK = 1>
	container<T, DIM, 1> Array(const std::array<T, DIM>& array) {
		return container<T, DIM, 1>(array);
	}

	container<double, 3, 2> generate_rand_ort();
	container<double, 3, 2> generate_indent_ort();

	template<typename T, size_t DIM, size_t RANK>
	requires FloatPoint<T, DIM, RANK>
	class container : public std::array<T, (size_t)pow(DIM, RANK)>
	{
	private:
		size_t _size;

		void _copy(const T* ptr) {
			memcpy(this->data(), ptr, _size * sizeof(T));
		}

		void _copy(const container& c) {
			this->_size = c._size;
			memcpy(this->data(), c.data(), _size * sizeof(T));
		}

		template<size_t DIM, size_t RANK = 1>
		void _copy(const std::array<T, DIM>& arr) {
			this->_size = DIM;
			memcpy(this->data(), &arr, _size * sizeof(T));
		}

		void _move(container& c) {
			this->_size = c._size;  c._size = 0;
			static_cast<std::array<T, (size_t)pow(DIM, RANK)>&>(*this) = std::move(c);
		}

	public:
		size_t size() const { return _size; };

		void fill_rand() {
			for (size_t i = 0; i < _size; ++i) {
				(*this)[i] = static_cast<T>(unidistr(gen));
			}
		};

		void fill_value(const T& val) {
			std::fill(this->begin(), this->end(), val);
		};

		void fill_value(tens::FILL_TYPE type) {
			auto& ref = *this;
			switch (type)
			{
			case tens::FILL_TYPE::ZERO:
				fill_value(T(0));
				break;
			case tens::FILL_TYPE::RANDOM:
				fill_rand();
				break;
			case tens::FILL_TYPE::RANDOMSYMM:
				fill_rand();
				for (size_t i = DIM; i < (_size - DIM) / 2 + DIM; ++i) {
					ref[i] = ref[i + DIM] = (ref[i] + ref[i + DIM]) * 0.5;
				}
				break;
			case tens::FILL_TYPE::RANDOMUNIT:
				fill_rand();
				*this = get_normalize(*this);
				break;
			case tens::FILL_TYPE::INDENT:
				fill_value(T(0));
				for (size_t i = 0; i < DIM; ++i) {
					ref[i] = T(1);
				}
				break;
			default:
				break;
			}
		};

		container() : std::array<T, pow(DIM, RANK)>(), _size(RANK != 0 ? (size_t)pow(DIM, RANK) : DIM) {};

		container(const std::array<T, pow(DIM, RANK)>& arr) : std::array<T, pow(DIM, RANK)>(arr), _size(RANK != 0 ? (size_t)pow(DIM, RANK) : DIM) {};

		container(FILL_TYPE type) : std::array<T, (size_t)pow(DIM, RANK)>(), _size(RANK != 0 ? (size_t)pow(DIM, RANK) : DIM) {
			fill_value(type);
		};
	
		container(const T& val) : std::array<T, (size_t)pow(DIM, RANK)>(), _size(RANK != 0 ? (size_t)pow(DIM, RANK) : DIM) {
			fill_value(val);
		};

		container(const container& c) : std::array<T, (size_t)pow(DIM, RANK)>(c), _size(c._size) {
			_copy(c);
		};

		container(container&& c) noexcept : std::array<T, (size_t)pow(DIM, RANK)>(std::move(c)), _size(c._size) {};

		operator T() const { 
			if (_size == 1) {
				return (*this)[0];
			}
			throw ErrorAccess::NoCastScalar();
		}

		inline container& operator= (const T value) {
			fill_value(value);
			return *this;
		}

		inline container& operator= (const container& rhs) {
			this->_copy(rhs);
			return *this;
		}

		inline container& operator= (container&& rhs) noexcept {
			this->_move(rhs);
			return *this;
		}

		[[nodiscard]] inline friend container<T, DIM, RANK> operator + (const container<T, DIM, RANK>& lhs, const container<T, DIM, RANK>& rhs) {
			container<T, DIM, RANK> nhs;
			for (size_t i = 0; i < lhs.size(); ++i)
				nhs[i] = lhs[i] + rhs[i];
			return nhs;
		}

		[[nodiscard]] inline friend container<T, DIM, RANK> operator - (const container<T, DIM, RANK>& lhs, const container<T, DIM, RANK>& rhs) {
			container<T, DIM, RANK> nhs;
			for (size_t i = 0; i < lhs.size(); ++i)
				nhs[i] = lhs[i] - rhs[i];
			return nhs;
		}

		[[nodiscard]] inline friend container<T, DIM, RANK> operator * (const container<T, DIM, RANK>& lhs, const T& mul) {
			container<T, DIM, RANK> nhs(lhs);
			for (size_t i = 0; i < lhs.size(); ++i)
				nhs[i] *= mul;
			return nhs;
		}

		[[nodiscard]] inline friend container<T, DIM, RANK> operator / (const container<T, DIM, RANK>& lhs, const T& div) {
			container<T, DIM, RANK> nhs(lhs);
			const T mul = T(1) / div;
#ifdef _DEBUG
			if (math::is_small_value(div)) {
				throw new ErrorMath::DivisionByZero();
			}
#endif
			for (size_t i = 0; i < lhs.size(); ++i)
				nhs[i] *= mul;
			return nhs;
		}

		[[nodiscard]] inline friend container<T, DIM, RANK> operator * (const T& mul, const container<T, DIM, RANK>& rhs) {
			return rhs * mul;
		}

		container& operator += (const container& rhs) {
			container& lhs = *this;
			for (size_t i = 0; i < _size; ++i)
				lhs[i] += rhs[i];
			return lhs;
		}

		container& operator -= (const container& rhs) {
			container& lhs = *this;
			for (size_t i = 0; i < _size; ++i)
				lhs[i] -= rhs[i];
			return lhs;
		}

		container& operator *= (const T& mul) {
			container& lhs = *this;
			for (size_t i = 0; i < _size; ++i)
				lhs[i] *= mul;
			return lhs;
		}

		container& operator *= (const container<T, DIM, RANK>& rhs) {
			container& lhs = *this;
			*this = lhs * rhs;
			return *this;
		}

		container<T, DIM, RANK>& operator /= (const T& div) {
#ifdef _DEBUG
			if (math::is_small_value(div)) {
				throw new ErrorMath::DivisionByZero();
			}
#endif
			const T mul = T(1) / div;
			container& lhs = *this;
			for (size_t i = 0; i < _size; ++i)
				lhs[i] *= mul;
			return lhs;
		}

		friend bool operator == (const container<T, DIM, RANK>& lhs, const container<T, DIM, RANK>& rhs) {
			const auto diff = lhs - rhs;
			if (math::is_small_value(diff.get_norm())) {
				return true;
			}
			return false;
		}
		T trace() const {
			if (RANK == 1) return *this;
			if (RANK == 2 && DIM == 3) {
				T trace(0);
				for (size_t i = 0; i < DIM; i++)	{
					trace += (*this)[i];
				}
				return trace;
			}
			throw NoImplemetationYet();
		}

		[[nodiscard]] container<T, DIM, RANK> transpose() const {
			if (RANK == 1) return *this;
			if (RANK == 2 && DIM == 3) {
				container<T, DIM, RANK> nhs(*this);
				std::swap(nhs[3], nhs[6]);
				std::swap(nhs[4], nhs[7]);
				std::swap(nhs[5], nhs[8]);
				return nhs;
			}
			throw NoImplemetationYet();
		}

		[[nodiscard]] container<T, DIM, RANK> symmetrize() const {
			if (RANK == 1) return *this;
			if (RANK == 2 && DIM == 3) {
				container<T, DIM, RANK> nhs(*this);
				nhs[3] = (nhs[3] + nhs[6])*T(0.5); nhs[6] = nhs[3];
				nhs[4] = (nhs[4] + nhs[7])*T(0.5); nhs[7] = nhs[4];
				nhs[5] = (nhs[5] + nhs[8])*T(0.5); nhs[8] = nhs[5];
				return nhs;
			}
			throw NoImplemetationYet();
		}
		
		T det() const {
			if (RANK != 2) {
				throw ErrorMath::ShapeMismatch();
			} else {
				if (DIM == 3) {
					return math::dim3::det_mat(this->data());
				}
				throw NoImplemetationYet();
			}
		}

		[[nodiscard]] container<T, DIM, RANK> inverse() const {
			if (DIM == 3) {
				container<T, DIM, RANK> inv_matr;
				math::dim3::inv_mat(this->data(), inv_matr.data());
				return inv_matr;
			} else {
				// for any dim matrix
			}
			throw NoImplemetationYet();
		}

		friend void inverse(container<T, DIM, RANK>& m) {
			if (DIM == 3) {
				container<T, DIM, RANK> inv_matr;
				math::dim3::inv_mat(m.data(), inv_matr.data());
				m = inv_matr;
				return;
			}
			else {
				// for any dim matrix
			}
			throw NoImplemetationYet();
		}

		T get_norm() const {
			T norm = T(0);
			const auto& arr = *this;
			for (size_t idx = 0; idx < _size; idx++){
				norm += arr[idx]*arr[idx];
			}
			return sqrt(norm);
		}

		[[nodiscard]] friend static container<T, DIM, RANK> get_normalize(const container<T, DIM, RANK>& m) {
			if (RANK == 1) {
				return m / m.get_norm();
			}
			throw NoImplemetationYet();
		}

		friend static void normalize(container<T, DIM, RANK>& m) {
			if (RANK == 1) {
				m /= m.get_norm();
				return;
			}
			throw NoImplemetationYet();
		}

		friend static void symmetrize(container<T, DIM, RANK>& m) {
			m = m.symmetrize();
		}

		// lhs : rhsT
		friend T convolution_transp(const container<T, DIM, RANK>& lhs, const container<T, DIM, RANK>& rhs) {
			if (RANK == 2 && DIM == 3) {
				return math::dim3::mat_conv_transp(lhs.data(), rhs.data());
			}
			throw NoImplemetationYet();
		}

		// lhs * rhsT
		friend  container<T, DIM, RANK> mat_scal_mat_transp(const container<T, DIM, RANK>& lhs, const  container<T, DIM, RANK>& rhs) {
			if (RANK == 2 && DIM == 3) {
				container<T, DIM, RANK> nhs;
				math::dim3::mat_scal_mat_transp(lhs.data(), rhs.data(), nhs.data());
				return nhs;
			}
			throw NoImplemetationYet();
		}

		template<typename T, size_t DIM, size_t RANK>
		friend std::pair<container<T, DIM, RANK>, container<T, DIM, RANK>> eigen(const tens::container<T, DIM, RANK>& M);
	};

	template<typename T, size_t DIM>
	void order_vectors(std::array<std::pair<tens::container<T, DIM, 1>, std::pair<T, size_t>>, DIM>& vectors) {

	}
	template<typename T, size_t DIM, size_t RANK>
	std::pair<container<T, DIM, RANK>, container<T, DIM, RANK>> eigen(const tens::container<T, DIM, RANK>& M) {
		// ------- for DIM == 3 : {00, 11, 22, 12, 02, 01, 21, 20, 10} 
#ifdef _DEBUG
		if (DIM != 3) {
			throw NoImplemetationYet();
		}
#endif
		Eigen::Matrix3d m;
		m << M[0], M[5], M[4], M[8], M[1], M[3], M[7], M[6], M[2];
		auto es = Eigen::EigenSolver<Eigen::Matrix3d>(m, true);
		const auto& l = es.eigenvalues();
		const auto& v = es.eigenvectors();

		tens::container<T, DIM, RANK> comp;
		tens::container<T, DIM, RANK> basis;
		std::array<
			std::pair<tens::container<T, DIM, 1>, 
			std::pair<T, size_t>
			>, DIM> vectors;

		comp[0] = l[0].real(); comp[1] = l[1].real(); comp[2] = l[2].real();
		vectors[0].first[0] = v(0).real(); vectors[0].first[1] = v(1).real(); vectors[0].first[2] = v(2).real();
		vectors[1].first[0] = v(3).real(); vectors[1].first[1] = v(4).real(); vectors[1].first[2] = v(5).real();
		vectors[2].first[0] = v(6).real(); vectors[2].first[1] = v(7).real(); vectors[2].first[2] = v(8).real();

		// ordering vectors: max element position is a number of basis vector
		// step 1: calc max and pos of max
		for (auto& v : vectors) {
			auto max_iter = std::max_element(v.first.begin(), v.first.end());
			auto min_iter = std::min_element(v.first.begin(), v.first.end());
			T max_value = *max_iter;
			T min_value = *min_iter;
			v.second.first = fabs(max_value);
			v.second.second = std::distance(v.first.begin(), max_iter);
			if (fabs(min_value) > fabs(max_value)) {
				v.first *= T(-1);
				v.second.first = fabs(min_value);
				v.second.second = std::distance(v.first.begin(), min_iter);
			}
		}
		// swap for ordering
		for (size_t num = 0; num < DIM; ++num) {
			size_t pos = vectors[num].second.second;
			if (pos != num) {
				std::swap(vectors[num], vectors[pos]);
				std::swap(comp[num], comp[pos]);
			}
		}

		basis[0] = vectors[0].first[0]; basis[5] = vectors[0].first[1]; basis[4] = vectors[0].first[2];
		basis[8] = vectors[1].first[0]; basis[1] = vectors[1].first[1]; basis[3] = vectors[1].first[2];
		basis[7] = vectors[2].first[0]; basis[6] = vectors[2].first[1]; basis[2] = vectors[2].first[2];

#ifdef _DEBUG
		check_ort(basis);
#endif
		return { comp , basis };
	}

	template<typename T, size_t DIM, size_t RANK = 2>
	std::array<container<T, DIM, 1>, DIM> slice_basis_to_vects(const tens::container<T, DIM, RANK>& basis) {
#ifdef _DEBUG
		if (DIM != 3)
			throw NoImplemetationYet();
#endif
		std::array<container<T, DIM, 1>, DIM> vectors;
		vectors[0] = container<T, DIM, 1>(std::array<T, DIM>{basis[0], basis[5], basis[4]});
		vectors[1] = container<T, DIM, 1>(std::array<T, DIM>{basis[8], basis[1], basis[3]});
		vectors[2] = container<T, DIM, 1>(std::array<T, DIM>{basis[7], basis[6], basis[2]});
		return vectors;
	}

	template<typename T, size_t DIM, size_t LRANK, size_t RRANK>
	[[nodiscard]] container<T, DIM, LRANK+RRANK-2> operator * (const container<T, DIM, LRANK>& lhs, const container<T, DIM, RRANK>& rhs) {
#ifdef _DEBUG
		if (RRANK + LRANK < 0 || RRANK < 1 || LRANK < 1) {
			throw NoImplemetationYet();
		}
#endif
		container<T, DIM, LRANK + RRANK - 2> nhs;
		if (LRANK == 2 && RRANK == 2) {
			math::dim3::mat_scal_mat(lhs.data(), rhs.data(), nhs.data());
		} else 	if (LRANK == 1 && RRANK == 2) {
			math::dim3::vect_scal_mat(lhs.data(), rhs.data(), nhs.data());
		} else 	if (LRANK == 2 && RRANK == 1) {
			math::dim3::mat_scal_vect(lhs.data(), rhs.data(), nhs.data());
		}
#ifdef _DEBUG
		else {
			throw NoImplemetationYet();
		}
#endif
		return nhs;
	}
	template<typename T, size_t DIM, size_t LRANK = 1, size_t RRANK = 1>
	[[nodiscard]] container<T, 1, 0> operator * (const container<T, DIM, 1>& lhs, const container<T, DIM, 1>& rhs) {
		container<T, 1, LRANK + RRANK - 2> scalar;
		scalar[0] = math::dim3::vect_scal_vect(lhs.data(), rhs.data());
		return scalar;
	}

	template<typename T, size_t DIM, size_t RANK>
	std::ostream& operator<<(std::ostream& out, const container<T, DIM, RANK>& cont) {
		size_t size = cont.size();
		out << "{ ";
		for (size_t row = 0; row < size - 1; row++)
			out << cont[row] << ", ";
		out << cont[cont.size() - 1] << " }";
		return out;
	};

	template <typename T>  using M3x3 = tens::container<T, 3, 2>;
	template <typename T, size_t N>  using MNxN = tens::container<T, N, 2>;
}


template <typename T, size_t DIM, size_t RANK = 2>
using Basis = std::shared_ptr<tens::container<T, DIM, RANK>>;

template<typename T, size_t DIM, size_t RANK = 2>
extern const Basis<T, DIM, RANK> GLOBAL_BASIS = std::make_shared<tens::container<T, DIM, RANK>>(tens::FILL_TYPE::INDENT);
template<typename T, size_t DIM>
extern const tens::container<T, DIM, 2> IDENT_MATRIX = tens::container<T, DIM, 2>(tens::FILL_TYPE::INDENT);
template<typename T>
extern const tens::container<T, 3, 2> I3x3 = IDENT_MATRIX<T,3>;
template<typename T, size_t DIM>
extern const tens::container<T, DIM, 2> ZERO_MATRIX = tens::container<T, DIM, 2>(tens::FILL_TYPE::ZERO);
