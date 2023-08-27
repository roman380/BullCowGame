#define _CRT_SECURE_NO_WARNINGS

#include <iostream>

#include "Engine.h"

int main()
{
    Game Game;
    Game.Secret.Create();
    //std::cout << "Secret: " << Game.Secret.ToString() << std::endl;
    for(; ; )
    {
        std::string Input;
        std::getline(std::cin, Input);
        if(Input == "q") // Quit
            break;
        if(Input == "r") // Restart
        {
            Game.Secret.Create();
            Game.QuestionVector.clear();
            std::cout << "-- Restarted, new game" << std::endl;
            continue;
        }
        Number Question;
        if(!Question.TryParse(Input))
        {
            std::cout << "Not a valid question: " << Input << std::endl;
            continue;
        }
        auto const Answer = Game.Secret.Ask(Question);
        Game.QuestionVector.emplace_back(Question);
        std::cout << "--" << std::endl;
        for(auto&& Question: Game.QuestionVector)
            std::cout << Question.ToString() << " " << Game.Secret.Ask(Question).ToString() << std::endl;
        if(Answer.Bulls == Question.ValueSize)
        {
            Game.Secret.Create();
            Game.QuestionVector.clear();
            std::cout << "-- Done, new game" << std::endl;
        }
    }
    return 0;
}

/*

- colorization
- output with emojis, such as https://emojipedia.org/keycap-digit-zero
- "?" + number pre-shows question against previous questions
- display how many combinations remain valid
- "+" and "-" to change your number card, "*" to reset it

*/