#define _CRT_SECURE_NO_WARNINGS

#include <iostream>

#include "Engine.h"

int main()
{
    // NOTE: 0518 without randomization
    std::srand(static_cast<unsigned int>(std::time(0))); // https://en.cppreference.com/w/cpp/numeric/random/srand
    Game Game;
    Game.Reset();
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
        if(Input.substr(0, 1) == "/") // Try
        {
            Number TryQuestion;
            if(!TryQuestion.TryParse(Input.substr(1)))
            {
                std::cout << "Not a valid question " << Input.substr(1) << std::endl;
                continue;
            }
            std::cout << "--" << std::endl;
            bool Match = true;
            for(auto&& Question: Game.QuestionVector)
            {
                auto const Answer = Game.Secret.Ask(Question);
                auto const TryAnswer = TryQuestion.Ask(Question);
                std::cout << Question.ToString() << " - " << Answer.ToString() << " - " << TryAnswer.ToString() << " against yours" << std::endl;
                Match &= Answer == TryAnswer;
            }
            if(Match)
                std::cout << TryQuestion.ToString() << " matches, go ahead" << std::endl;
            continue;
        }
        Number Question;
        if(!Question.TryParse(Input))
        {
            std::cout << "Not a valid question " << Input << std::endl;
            continue;
        }
        auto const Answer = Game.Secret.Ask(Question);
        Game.QuestionVector.emplace_back(Question);
        std::cout << "--" << std::endl;
        auto MatchVector = Number::All();
        for(auto&& Question: Game.QuestionVector)
        {
            auto const Answer = Game.Secret.Ask(Question);
            std::vector<Number> NewMatchVector;
            std::for_each(MatchVector.cbegin(), MatchVector.cend(), [&] (auto&& Match)
            { 
                if(Game.Secret.Ask(Question) == Match.Ask(Question))
                    NewMatchVector.emplace_back(Match);
            });
            std::cout << Question.ToString() << " - " << Answer.ToString() << " - " << MatchVector.size() << " -> " << NewMatchVector.size() << std::endl;
            MatchVector = std::move(NewMatchVector);
        }
        Game.MatchVector = MatchVector;
        if(Answer.Bulls == Question.ValueSize)
        {
            Game.Reset();
            std::cout << "-- Done, new game" << std::endl;
        }
    }
    return 0;
}

/*

- colorization
- output with emojis, such as https://emojipedia.org/keycap-digit-zero
- "+" and "-" to change your number card, "*" to reset it

*/
