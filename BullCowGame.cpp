#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <iomanip>
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

    std::cout << "q to quit, r to restart, u to undo, h for hint" << std::endl;
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

    auto const OutputBoard = [&] (std::function<std::string(Combination const&)> QuestionComment = nullptr)
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
            if(QuestionComment)
            {
                auto const Comment = QuestionComment(Question);
                if(!Comment.empty())
                {
                    Stream << "\033[36m"; // Cyan
                    Stream << " // " << Comment;
                }
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
            if(Game.MatchVector.size() <= 24)
                for(auto&& Match: Game.MatchVector)
                {
                    std::ostringstream Stream;
                    Stream << Match.ToString();
                    Stream << "\033[36m"; // Cyan
                    Stream << " -> " << std::fixed << std::setprecision(2) << Game.AverageSameAnswer(Match);
                    Stream << "\033[0m"; // Default
                    std::cout << "  " << Stream.str() << std::endl;
                }
            continue;
        }
        if(Input.size() == Combination::ValueSize + 1 && Input[Combination::ValueSize] == '?') // Remaining matches, filtered, if 16 or less
        {
            std::optional<char> Pattern[Combination::ValueSize];
            bool Invalid = false;
            for(size_t Index = 0; Index < Combination::ValueSize; Index++)
            {
                auto const Character = Input[Index];
                if(Character == '-')
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
            if(MatchVector.size() <= 24)
                for(auto&& Match: MatchVector)
                {
                    std::ostringstream Stream;
                    Stream << Match.ToString();
                    Stream << "\033[36m"; // Cyan
                    Stream << " -> " << std::fixed << std::setprecision(2) << Game.AverageSameAnswer(Match);
                    Stream << "\033[0m"; // Default
                    std::cout << "  " << Stream.str() << std::endl;
                }
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
        if(Input == "u") // Undo
        {
            if(!Game.QuestionVector.empty())
                Game.QuestionVector.erase(Game.QuestionVector.end() - 1);
            OutputBoard();
            continue;
        }
        if(Input == "h") // Hint
        {
            if(!Game.QuestionVector.empty() && !Game.MatchVector.empty())
            {
                std::vector<double> MeritVector;
                MeritVector.reserve(Game.MatchVector.size());
                std::for_each(Game.MatchVector.cbegin(), Game.MatchVector.cend(), [&] (auto&& Question) { MeritVector.emplace_back(Game.AverageSameAnswer(Question)); });
                auto const MeritIterator = std::min_element(MeritVector.cbegin(), MeritVector.cend());
                assert(MeritIterator != MeritVector.cend());
                auto const& Question = Game.MatchVector[std::distance(MeritVector.cbegin(), MeritIterator)];
                std::ostringstream Stream;
                Stream << "Try " << Question.ToString();
                Stream << "\033[36m"; // Cyan
                Stream << " -> " << std::fixed << std::setprecision(2) << Game.AverageSameAnswer(Question);
                Stream << "\033[0m"; // Default
                std::cout << Stream.str() << std::endl;
            }
            continue;
        }
        if(Input.size() == Combination::ValueSize + 1 && Input[Combination::ValueSize] == '/') // Try
        {
            Combination TryQuestion;
            if(!TryQuestion.TryParse(Input.substr(0, Combination::ValueSize)))
            {
                std::cout << "Not a valid combination " << Input.substr(0, Combination::ValueSize) << std::endl;
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
                {
                    //Stream << "match";
                    continue;
                }
                Stream << "\033[0m"; // Default
                std::cout << Stream.str() << std::endl;
                Match &= Answer == TryAnswer;
            }
            if(Match)
                std::cout << "Matches, go ahead" << std::endl;
            continue;
        }
        Combination Question;
        if(!Question.TryParse(Input))
        {
            std::cout << "Not a valid combination " << Input << std::endl;
            continue;
        }
        auto const Answer = Game.Secret.Ask(Question);
        std::string Comment;
        if(Answer.Bulls != Combination::ValueSize && !Game.QuestionVector.empty() && Game.MatchVector.size() < 256)
        {
            std::ostringstream Stream;
            Stream << "expected " << std::fixed << std::setprecision(2) << Game.AverageSameAnswer(Question);
            if(Game.MatchVector.size() < 128)
            {
                // WARN: This is not quite accurate, best question does not have to belong to one of the potentially possible combinations
                std::vector<double> MeritVector;
                MeritVector.reserve(Game.MatchVector.size());
                std::for_each(Game.MatchVector.cbegin(), Game.MatchVector.cend(), [&] (auto&& Question) { MeritVector.emplace_back(Game.AverageSameAnswer(Question)); });
                auto const MeritIterator = std::min_element(MeritVector.cbegin(), MeritVector.cend());
                assert(MeritIterator != MeritVector.cend());
                auto const& Question = Game.MatchVector[std::distance(MeritVector.cbegin(), MeritIterator)];
                Stream << ", best " << Game.AverageSameAnswer(Question) << " (" << Question.ToString() << ")";
            }
            Comment = Stream.str();
        }
        Game.QuestionVector.emplace_back(Question);
        if(Answer.Bulls != Question.ValueSize)
            Game.AutomaticUpdateCharacterStates();
        Game.MatchVector = OutputBoard([&] (auto const& BoardQuestion) -> std::string { return BoardQuestion == Question ? Comment : ""; });
        if(Answer.Bulls == Question.ValueSize)
        {
            Game.Reset();
            std::cout << "-- Done, new game" << std::endl;
        }
    }
    return 0;
}

/*

- optimal move, how your move looks compared to optimal in terms how much of entropy is reduced
- list remaining with % for how optimal the attempt would be

*/
