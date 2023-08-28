#pragma once

#include <cassert>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <utility>
#include <functional>
#include <optional>

struct Answer
{
	bool operator==(Answer const& Value) const
	{
		return Bulls == Value.Bulls && Cows == Value.Cows;
	}
	bool operator!=(Answer const& Value) const
	{
		return !(*this == Value);
	}

	std::string ToString() const
	{
		char Result[16];
		sprintf(Result, "%u %u", Bulls, Cows);
		return Result;
	}

	unsigned int Bulls = 0;
	unsigned int Cows = 0;
};

struct Combination
{
	bool operator==(Combination const& Value) const
	{
		for(size_t Index = 0; Index < std::size(this->Value); Index++)
			if(this->Value[Index] != Value.Value[Index])
				return false;
		return true;
	}
	bool operator!=(Combination const& Value) const
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
	std::string ToString(std::function<std::string(std::optional<size_t> Index, char Value)> Colorize) const
	{
		assert(Colorize);
		std::ostringstream Stream;
		for(size_t Index = 0; Index < std::size(Value); Index++)
			Stream << Colorize(Index, Value[Index]);
		return Stream.str();
	}
	static bool ValidCharacter(char Value)
	{
		return Value >= '0' && Value <= '9';
	}
	bool Valid() const
	{
		static_assert(ValueSize == sizeof Value / sizeof *Value);
		static_assert(ValueSize <= 10);
		for(auto&& Element : Value)
			if(!ValidCharacter(Element))
				return false;
		for(size_t IndexA = 0; IndexA < std::size(Value) - 1; IndexA++)
			for(size_t IndexB = IndexA + 1; IndexB < std::size(Value); IndexB++)
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
	Answer Ask(Combination const& Question) const
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
	static std::vector<Combination> All()
	{
		std::vector<Combination> Result;
		Combination Value;
		for(size_t Index = 0; Index < ValueSize; Index++)
			Value.Value[Index] = static_cast<char>('0' + Index);
		assert(Value.Valid());
		Result.emplace_back(Value);
		for(; ; )
		{
			size_t Index = ValueSize;
			for(; Index > 0; Index--)
			{
				Value.Value[Index - 1]++;
				if(Value.Value[Index - 1] <= '9')
					break;
				Value.Value[Index - 1] = '0';
			}
			if(Index == 0)
				break;
			if(Value.Valid())
				Result.emplace_back(Value);
		}
		return Result;
	}
	bool Has(char Value) const
	{
		return std::any_of(std::cbegin(this->Value), std::cend(this->Value), [=] (auto Element) { return Element == Value; });
	}

	static size_t constexpr const ValueSize = 4;
	char Value[ValueSize];
};

enum class CharacterState
{
	Default,
	Absent,
	Present,
};

struct Game
{
	void Reset()
	{
		Secret.Create();
		QuestionVector.clear();
		MatchVector = Combination::All();
		ResetCharacterStates();
	}
	void ResetCharacterStates()
	{
		for(auto&& Element: CharacterStates)
			Element = CharacterState::Default;
		AutomaticUpdateCharacterStates();
	}
	static unsigned int DistinctCount(Combination const& QuestionA, Combination const& QuestionB)
	{
		unsigned int Count[10] { };
		for(auto&& Value: QuestionA.Value)
			Count[Value - '0']++;
		for(auto&& Value: QuestionB.Value)
			Count[Value - '0']++;
		return static_cast<unsigned int>(std::count_if(std::cbegin(Count), std::cend(Count), [] (auto Element) { return Element > 0; }));
	}
	void AutomaticUpdateCharacterStates()
	{
		for(auto&& Question: QuestionVector)
		{
			auto const Answer = Secret.Ask(Question);
			if(Answer.Bulls == 0 && Answer.Cows == 0)
			{
				for(auto&& Element: Question.Value)
					CharacterStates[Element - '0'] = CharacterState::Absent;
			} else 
			if(Answer.Bulls + Answer.Cows == Combination::ValueSize)
			{
				for(auto&& Element: CharacterStates)
					Element = CharacterState::Absent;
				for(auto&& Element: Question.Value)
					CharacterStates[Element - '0'] = CharacterState::Present;
			}
		}
		if(QuestionVector.size() >= 2)
		{
			for(size_t IndexA = 0; IndexA < QuestionVector.size() - 1; IndexA++)
				for(size_t IndexB = IndexA + 1; IndexB < QuestionVector.size(); IndexB++)
				{
					auto const& QuestionA = QuestionVector[IndexA];
					auto const& QuestionB = QuestionVector[IndexB];
					auto const AnswerA = Secret.Ask(QuestionA);
					auto const AnswerB = Secret.Ask(QuestionB);
					auto const Distinct = DistinctCount(QuestionA, QuestionB);
					if(Distinct == Combination::ValueSize * 2)
					{
						auto const Count = AnswerA.Bulls + AnswerA.Cows + AnswerB.Bulls + AnswerB.Cows;
						if(Count == Combination::ValueSize)
						{
							for(char Character = '0'; Character <= '9'; Character++)
								if(!QuestionA.Has(Character) && !QuestionB.Has(Character))
									CharacterStates[Character - '0'] = CharacterState::Absent;
						} else
						if(Count == 10 - Combination::ValueSize * 2)
						{
							for(char Character = '0'; Character <= '9'; Character++)
								if(!QuestionA.Has(Character) && !QuestionB.Has(Character))
									CharacterStates[Character - '0'] = CharacterState::Present;
						}
					} else
					if(Distinct == Combination::ValueSize + 1)
					{
						// SUGG: Same for difference of 2
						if(AnswerA.Bulls + AnswerA.Cows + 1 == AnswerB.Bulls + AnswerB.Cows) // A's difference is absent, B's difference is present
						{
							for(char Character = '0'; Character <= '9'; Character++)
								if(QuestionA.Has(Character) && !QuestionB.Has(Character))
									CharacterStates[Character - '0'] = CharacterState::Absent;
								else if(!QuestionA.Has(Character) && QuestionB.Has(Character))
									CharacterStates[Character - '0'] = CharacterState::Present;
						} else
						if(AnswerA.Bulls + AnswerA.Cows == AnswerB.Bulls + AnswerB.Cows + 1) // See above, vice versa
						{
							for(char Character = '0'; Character <= '9'; Character++)
								if(QuestionA.Has(Character) && !QuestionB.Has(Character))
									CharacterStates[Character - '0'] = CharacterState::Present;
								else if(!QuestionA.Has(Character) && QuestionB.Has(Character))
									CharacterStates[Character - '0'] = CharacterState::Absent;
						}
					}
				}
		}
	}
	bool DefaultCharacterStates() const
	{
		return std::all_of(std::cbegin(CharacterStates), std::cend(CharacterStates), [] (auto&& Element) { return Element == CharacterState::Default; });
	}

	Combination Secret;
	std::vector<Combination> QuestionVector;
	std::vector<Combination> MatchVector;
	CharacterState CharacterStates[10];
};
