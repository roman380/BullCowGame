#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <windows.h>

#include "Engine.h"

int main()
{
    auto const OutputConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD OutputConsoleMode;
    GetConsoleMode(OutputConsoleHandle, &OutputConsoleMode);
    SetConsoleMode(OutputConsoleHandle, OutputConsoleMode | ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    // NOTE: https://learn.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences#text-formatting

    // NOTE: 0518 without randomization
    std::srand(static_cast<unsigned int>(std::time(0))); // https://en.cppreference.com/w/cpp/numeric/random/srand
    Game Game;
    Game.Reset();

    std::cout << "q to quit, r to restart, u to undo" << std::endl;
    std::cout << "? shows remaining, ?? shows the secret" << std::endl;
    std::cout << "+N, -N marks as present or absent, *N resets, * resets all" << std::endl;
    std::cout << "/<number> tries a combination against previous attempts" << std::endl;
    std::cout << std::endl;
    std::cout << "-- Go ahead" << std::endl;

    auto const Colorize = [&] (std::optional<size_t>, char Value)
    {
        assert(Value >= '0' && Value <= '9');
        std::ostringstream Stream;
        Stream << "\033[1m"; // Bold
        switch(Game.CharacterStates[Value - '0'])
        {
        case CharacterState::Absent:
            Stream << "\033[31m"; // Red
            Stream << "\033[7m"; // Invert
            Stream << Value;
            break;
        case CharacterState::Present:
            Stream << "\033[32m"; // Green
            Stream << Value;
            break;
        default:
            Stream << Value;
        }
        Stream << "\033[0m"; // Default
        return Stream.str();
    };

    auto const OutputBoard = [&]
    {
        std::cout << "--" << std::endl;
        if(!Game.DefaultCharacterStates())
        {
            std::ostringstream Stream;
            for(char Character = '0'; Character <= '9'; Character++)
                Stream << Colorize(std::nullopt, Character);
            std::cout << "[" << Stream.str() << "]" << std::endl;
        }
        unsigned int QuestionIndex = 0;
        auto MatchVector = Combination::All();
        for(auto&& Question: Game.QuestionVector)
        {
            auto const Answer = Game.Secret.Ask(Question);
            std::vector<Combination> NewMatchVector;
            std::for_each(MatchVector.cbegin(), MatchVector.cend(), [&] (auto&& Match)
            { 
                if(Game.Secret.Ask(Question) == Match.Ask(Question))
                    NewMatchVector.emplace_back(Match);
            });
            std::ostringstream Stream;
            Stream << ++QuestionIndex << ": ";
            Stream << Question.ToString(Colorize) << " - " << Answer.ToString();
            if(Answer.Bulls != Question.ValueSize)
            {
                Stream << "\033[36m"; // Cyan
                Stream << " (" << NewMatchVector.size() << ")";
            }
            Stream << "\033[0m"; // Default
            std::cout << Stream.str() << std::endl;
            MatchVector = std::move(NewMatchVector);
        }
        return MatchVector;
    };

    for(; ; )
    {
        std::string Input;
        std::getline(std::cin, Input);
        if(Input.empty())
            continue;
        if(Input == "q") // Quit
            break;
        if(Input == "r") // Restart
        {
            Game.Reset();
            std::cout << "-- Restarted, new game" << std::endl;
            continue;
        }
        if(Input == "??") // Secret
        {
            std::cout << "Secret is " << Game.Secret.ToString() << std::endl;
            continue;
        }
        if(Input == "?") // Remaining matches, if 16 or less
        {
            std::cout << Game.MatchVector.size() << " remaining" << std::endl;
            static size_t constexpr const MatchSizeThreshold = 16;
            if(Game.MatchVector.size() <= MatchSizeThreshold)
                for(auto&& Match: Game.MatchVector)
                    std::cout << "  " << Match.ToString() << std::endl;
            continue;
        }
        if(Input.size() == 1 + Combination::ValueSize && Input[0] == '?') // Remaining matches, filtered, if 16 or less
        {
            std::optional<char> Pattern[Combination::ValueSize];
            bool Invalid = false;
            for(size_t Index = 0; Index < Combination::ValueSize; Index++)
            {
                auto const Character = Input[1 + Index];
                if(Character == '_')
                    continue;
                if(!Combination::ValidCharacter(Character))
                {
                    Invalid = true;
                    break;
                }
                Pattern[Index] = Character;
            }
            if(Invalid)
                continue;
            std::vector<Combination> MatchVector;
            for(auto&& Combination: Game.MatchVector)
            {
                bool Match = true;
                for(size_t Index = 0; Index < Combination::ValueSize && Match; Index++)
                {
                    if(!Pattern[Index].has_value())
                        continue;
                    Match &= Pattern[Index].value() == Combination.Value[Index];
                }
                if(Match)
                    MatchVector.emplace_back(Combination);
            }
            std::cout << MatchVector.size() << " remaining" << std::endl;
            static size_t constexpr const MatchSizeThreshold = 16;
            if(MatchVector.size() <= MatchSizeThreshold)
                for(auto&& Match: MatchVector)
                    std::cout << "  " << Match.ToString() << std::endl;
            continue;
        }
        if(Input == "*") // Reset character states
        {
            Game.ResetCharacterStates();
            OutputBoard();
            continue;
        }
        if(strchr("+-*", Input.substr(0, 1)[0])) // Update character states
        {
            for(size_t Position = 0; ; Position += 2)
            {
                if(Position + 2 > Input.size())
                    break;
                auto const Operation = Input[Position + 0];
                if(!strchr("+-*", Operation))
                    break;
                auto const Character = Input[Position + 1];
                if(Character < '0' || Character > '9')
                    break;
                if(Operation != '*')
                    Game.CharacterStates[Character - '0'] = (Operation == '+') ? CharacterState::Present : CharacterState::Absent;
                else
                    Game.CharacterStates[Character - '0'] = CharacterState::Default;
            }
            OutputBoard();
            continue;
        }
        if(Input.substr(0, 1) == "-" && Input.size() == 2) // Set absent
        {
            auto const Character = Input.substr(1, 1)[0];
            if(Combination::ValidCharacter(Character))
                Game.CharacterStates[Character - '0'] = CharacterState::Absent;
            OutputBoard();
            continue;
        }
        if(Input.substr(0, 1) == "*" && Input.size() == 2) // Set unknown
        {
            auto const Character = Input.substr(1, 1)[0];
            if(Combination::ValidCharacter(Character))
                Game.CharacterStates[Character - '0'] = CharacterState::Default;
            OutputBoard();
            continue;
        }
        if(Input == "u") // Undo
        {
            if(!Game.QuestionVector.empty())
                Game.QuestionVector.erase(Game.QuestionVector.end() - 1);
            OutputBoard();
            continue;
        }
        if(Input.substr(0, 1) == "/") // Try
        {
            Combination TryQuestion;
            if(!TryQuestion.TryParse(Input.substr(1)))
            {
                std::cout << "Not a valid combination " << Input.substr(1) << std::endl;
                continue;
            }
            std::cout << "--" << std::endl;
            bool Match = true;
            for(auto&& Question: Game.QuestionVector)
            {
                auto const Answer = Game.Secret.Ask(Question);
                auto const TryAnswer = TryQuestion.Ask(Question);
                std::ostringstream Stream;
                Stream << Question.ToString() << " - " << Answer.ToString() << " - " << TryAnswer.ToString() << " ";
                if(Answer != TryAnswer)
                {
                    Stream << "\033[31m"; // Red
                    Stream << "\033[7m"; // Invert
                    Stream << "mismatch";
                } else
                    Stream << "match";
                Stream << "\033[0m"; // Default
                std::cout << Stream.str() << std::endl;
                Match &= Answer == TryAnswer;
            }
            continue;
        }
        Combination Question;
        if(!Question.TryParse(Input))
        {
            std::cout << "Not a valid combination " << Input << std::endl;
            continue;
        }
        auto const Answer = Game.Secret.Ask(Question);
        Game.QuestionVector.emplace_back(Question);
        if(Answer.Bulls != Question.ValueSize)
            Game.AutomaticUpdateCharacterStates();
        Game.MatchVector = OutputBoard();
        if(Answer.Bulls == Question.ValueSize)
        {
            Game.Reset();
            std::cout << "-- Done, new game" << std::endl;
        }
    }
    return 0;
}

/*

*/
