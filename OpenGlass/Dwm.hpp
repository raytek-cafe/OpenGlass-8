#pragma once
#include "framework.hpp"
#include "cpprt.hpp"

namespace OpenGlass::DWM
{
	// actually gsl::span
	template <typename T>
	struct span
	{
		size_t length;
		T* data;

		auto views() const
		{
			return std::span<T>(data, length);
		}
	};

	template <typename T>
	struct DynArray
	{
		T* data;
		T* buffer;
		UINT bufferCapacity;
		UINT capacity;
		UINT size;

		auto views() const
		{
			return std::span<T>(data, size);
		}
	};

	template <typename T>
	struct MyDynArrayImpl : DynArray<T>
	{
		[[nodiscard]] void* operator new[](
			size_t size
		)
		{
			auto memory = HeapAlloc(OpenGlass::Util::g_processHeap, 0, size);
			THROW_LAST_ERROR_IF_NULL(memory);
			return memory;
		}
		void operator delete[](
			void* ptr
		) noexcept
		{
			FAIL_FAST_IF_NULL(ptr);
			HeapFree(OpenGlass::Util::g_processHeap, 0, ptr);
		}

		MyDynArrayImpl()
		{
			this->capacity = 8;
			this->data = new T[this->capacity];
			this->size = 0;
		}
		~MyDynArrayImpl()
		{
			delete[] this->data;
			this->data = nullptr;
			this->capacity = this->size = 0;
		}

		void Clear()
		{
			if (this->size != 0)
			{
				this->capacity = 8;
				delete[] this->data;
				this->data = new T[this->capacity];
				this->size = 0;
			}
		}
		void Add(const T& object)
		{
			auto newSize = this->size + 1u;
			if (newSize < this->size)
			{
				FAIL_FAST_HR(static_cast<HRESULT>(0x80070216ul));
			}
			else
			{
				auto bufferSize = this->size * sizeof(T);
				if (newSize > this->capacity)
				{
					auto tmp = std::unique_ptr<T[]>(this->data);

					this->capacity *= 2;
					this->data = new T[this->capacity];
					memcpy_s(this->data, bufferSize, tmp.get(), bufferSize);
				}

				*reinterpret_cast<T*>(reinterpret_cast<ULONG_PTR>(this->data) + bufferSize) = object;
				this->size = newSize;
			}
		}
		template <typename... Args>
		void AddInPlace(Args&&... args)
		{
			auto newSize = this->size + 1u;
			if (newSize < this->size)
			{
				FAIL_FAST_HR(static_cast<HRESULT>(0x80070216ul));
			}
			else
			{
				auto bufferSize = this->size * sizeof(T);
				if (newSize > this->capacity)
				{
					auto tmp = std::unique_ptr<T[]>(this->data);

					this->capacity *= 2;
					this->data = new T[this->capacity];
					memcpy_s(this->data, bufferSize, tmp.get(), bufferSize);
				}

				*reinterpret_cast<T*>(reinterpret_cast<ULONG_PTR>(this->data) + bufferSize) = T{ std::forward<Args>(args)... };
				this->size = newSize;
			}
		}
	};

	struct MilPoint2D
	{
		double x;
		double y;
	};
	struct MilRectD
	{
		double left;
		double top;
		double right;
		double bottom;
	};
	struct MilSizeD
	{
		double width;
		double height;
	};
	enum MIL_RESOURCE_TYPE : UINT {};
	enum class MilBrushMappingMode
	{
		Absolute,
		RelativeToBoundingBox
	};
	enum class MilBitmapInterpolationMode
	{
		NearestNeighbor,
		Linear,
		Cubic,
		Fant,
		TriLinear,
		Anisotropic,
		SuperSample
	};
	enum class MilStretch
	{
		None,
		Fill,
		Uniform,
		UniformToFill
	};
	struct MilColorTransform
	{
		union
#pragma warning(suppress : 4201)
		{
			struct
#pragma warning(suppress : 4201)
			{
				FLOAT _11, _12, _13, _14, _15;
				FLOAT _21, _22, _23, _24, _25;
				FLOAT _31, _32, _33, _34, _35;
				FLOAT _41, _42, _43, _44, _45;
				FLOAT _51, _52, _53, _54, _55;
			} DUMMYSTRUCTNAME;

			FLOAT m[5][5];
		} DUMMYUNIONNAME;
	};
	using MilTransform = D2D1_MATRIX_3X2_F;
	using MilTransform3D = D2D1_MATRIX_4X3_F;

	using HMIL_CONNECTION = HANDLE;
	struct IDwmChannel {};
	struct IDwmWindow {};
	// since windows 11 24h2
	struct IDwmChannelProvider : IUnknown 
	{
		virtual HRESULT STDMETHODCALLTYPE CreateMilResource(MIL_RESOURCE_TYPE, UINT*, IUnknown**) = 0;
		virtual HRESULT STDMETHODCALLTYPE CreateSharedMilResource(MIL_RESOURCE_TYPE, UINT*, void**, IUnknown**) = 0;
		virtual HRESULT STDMETHODCALLTYPE DuplicateSharedMilResource(void*, MIL_RESOURCE_TYPE, bool, UINT*, IUnknown**) = 0;
		virtual UINT STDMETHODCALLTYPE GetChannelHandle(void) = 0;
		virtual void STDMETHODCALLTYPE Lock(void) = 0;
		virtual void STDMETHODCALLTYPE ReleaseMilResource(IUnknown*) = 0;
		virtual HRESULT STDMETHODCALLTYPE InternalCommit(void*) = 0;
		virtual void STDMETHODCALLTYPE Unlock(void) = 0;
	};
}