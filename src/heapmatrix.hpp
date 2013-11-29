#include <vector>
#include <utility>
#include <cstddef>

template<class T>
class HeapMatrix
{
public:
	typedef std::vector<T> Storage;
	typedef typename Storage::iterator Iterator;
	typedef typename Storage::const_iterator ConstIterator;
	typedef typename Storage::reference Reference;
	typedef typename Storage::const_reference ConstReference;

	template <class IterType=Iterator,
		class RefType=Reference,
		class ConstIterType=ConstIterator,
		class ConstRefType=ConstReference>
	class Row
	{
	public:
		Row(IterType iter, size_t size):
			mSize(size),
			mIter(iter)
		{}

		Row(const Row&) = default;

		RefType operator[](size_t col)
		{
			return mIter[col];
		}

		ConstRefType operator[](size_t col) const
		{
			return mIter[col];
		}

		bool operator==(const Row& other)
		{
			return mIter == other.mIter;
		}

		bool operator!=(const Row& other)
		{
			return mIter != other.mIter;
		}

		// Container operations, to be able to
		// use in for(auto i:...)
		IterType begin()
		{
			return mIter;
		}

		ConstIterType begin() const
		{
			return mIter;
		}

		IterType end()
		{
			return mIter + mSize;
		}

		ConstIterType end() const
		{
			return mIter + mSize;
		}

		// Iterator operations:

		// This feels ugly, but a Row is its own
		// iterator...

		Row& operator*()
		{
			return *this;
		}

		Row& operator++()
		{
			mIter += mSize;
			return *this;
		}

		Row& operator++(int)
		{
			Row copy(*this);
			mIter += mSize;
			return copy;
		}

		Row& operator--()
		{
			mIter -= mSize;
			return *this;
		}

		Row& operator--(int)
		{
			Row copy(*this);
			mIter -= mSize;
			return copy;
		}

	private:
		size_t mSize;
		IterType mIter;
	};

	typedef const Row<ConstIterator, ConstReference> ConstRow;

	HeapMatrix() = default;
	HeapMatrix(const HeapMatrix&) = default;

	HeapMatrix(size_t rows, size_t cols, const T& value = T()):
		mRows(rows), mCols(cols),
		mVec(rows*cols, value)
	{}

	HeapMatrix(HeapMatrix &&other):
		mRows(other.mRows), mCols(other.mCols),
		mVec(std::move(other.mVec))
	{}

	void resize(size_t rows, size_t cols, const T& value = T())
	{
		mRows = rows;
		mCols = cols;
		mVec.resize(rows * cols, value);
	}

	Row<> operator[](size_t row)
	{
		return Row<>(mVec.begin() + row * mCols, mCols);
	}

	ConstRow operator[](size_t row) const
	{
		return ConstRow(mVec.begin() + row * mCols, mCols);
	}

	Row<> begin()
	{
		return Row<>(mVec.begin(), mCols);
	}

	ConstRow begin() const
	{
		ConstRow(mVec.begin(), mCols);
	}

	Row<> end()
	{
		return Row<>(mVec.end(), mCols);
	}

	ConstRow end() const
	{
		return ConstRow(mVec.end(), mCols);
	}

	size_t numRows() const
	{
		return mRows;
	}

	size_t numCols() const
	{
		return mCols;
	}

private:
	size_t mRows, mCols;
	Storage mVec;
};
