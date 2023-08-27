#pragma once

#include <cassert>
#include <vector>
#include <string>

struct Answer
{
	std::string ToString() const
	{
		char Result[16];
		sprintf(Result, "%u %u", Bulls, Cows);
		return Result;
	}

	unsigned int Bulls = 0;
	unsigned int Cows = 0;
};

struct Number
{
	bool operator==(Number const& Value) const
	{
		for(size_t Index = 0; Index < std::size(this->Value); Index++)
			if(this->Value[Index] != Value.Value[Index])
				return false;
		return true;
	}
	bool operator!=(Number const& Value) const
	{
		return !(*this == Value);
	}

	bool TryParse(std::string const& Value)
	{
		if(Value.size() != ValueSize)
			return false;
		for(size_t Index = 0; Index < std::size(this->Value); Index++)
			this->Value[Index] = Value[Index];
		return Valid();
	}
	std::string ToString() const
	{
		char Value[ValueSize + 1];
		std::strncpy(Value, this->Value, ValueSize);
		Value[ValueSize] = 0;
		return Value;
	}
	bool Valid() const
	{
		for(auto&& Element : Value)
			if(Element < '0' || Element > '9')
				return false;
		for(size_t IndexA = 0; IndexA < std::size(Value) - 1; IndexA++)
			for(size_t IndexB = IndexA + 1; IndexB < std::size(Value) - 1; IndexB++)
				if(Value[IndexA] == Value[IndexB])
					return false;
		return true;
	}
	void Create()
	{
		for(; ; )
		{
			for(size_t Index = 0; Index < std::size(Value); Index++)
				Value[Index] = '0' + (10 * std::rand() / RAND_MAX);
			if(Valid())
				break;
		}
	}
	Answer Ask(Number const& Question) const
	{
		assert(Valid() && Question.Valid());
		Answer Answer;
		for(size_t IndexA = 0; IndexA < std::size(Value); IndexA++)
			for(size_t IndexB = 0; IndexB < std::size(Value); IndexB++)
				if(Value[IndexA] == Question.Value[IndexB])
					if(IndexA == IndexB)
						Answer.Bulls++;
					else
						Answer.Cows++;
		assert(Answer.Bulls + Answer.Cows <= std::size(Value));
		return Answer;
	}

	static size_t constexpr const ValueSize = 4;
	char Value[4];
};

struct Game
{
	Number Secret;
	std::vector<Number> QuestionVector;
};
