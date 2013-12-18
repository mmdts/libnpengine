/* 
 * libnpengine: Nitroplus script interpreter
 * Copyright (C) 2013 Mislav Blažević <krofnica996@gmail.com>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * */
#include "drawable.hpp"
#include "game.hpp"
#include "resourcemgr.hpp"
#include "text.hpp"
#include "movie.hpp"

#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <boost/lexical_cast.hpp>

const std::function<int32_t(int32_t)> SpecialPosTable[SPECIAL_POS_NUM] =
{
  [] (int32_t x) { return WINDOW_WIDTH / 2 - x / 2; },
  [] (int32_t y) { return WINDOW_HEIGHT - y; },
  [] (int32_t y) { return WINDOW_HEIGHT / 2 - y / 2; },
  [] (int32_t x) { return 0; },
  [] (int32_t y) { return 0; },
  [] (int32_t y) { return 0; },
  [] (int32_t x) { return 0; }
};

sf::Texture* NsbInterpreter::LoadTextureFromFile(const string& File)
{
    uint32_t Size;
    char* pPixels = sResourceMgr->Read(File, &Size);
    if (NsbAssert(pPixels != nullptr, "Failed to load % pixels", File))
        return nullptr;

    sf::Texture* pTexture = new sf::Texture;
    if (NsbAssert(pTexture->loadFromMemory(pPixels, Size), "Failed to load pixels from % in memory", File))
    {
        delete pTexture;
        return nullptr;
    }
    return pTexture;
}

void NsbInterpreter::GLCreateTexture(int32_t Width, int32_t Height, const string& Color)
{
    if (sf::RenderTexture* pTexture = CacheHolder<sf::RenderTexture>::Read(HandleName))
        delete pTexture;

    sf::RenderTexture* pTexture = new sf::RenderTexture;
    pTexture->create(Width, Height);
    CacheHolder<sf::RenderTexture>::Write(HandleName, pTexture);
}

void NsbInterpreter::GLDrawToTexture(sf::RenderTexture* pTexture, int32_t x, int32_t y, const string& File)
{
    if (sf::Texture* pTempTexture = LoadTextureFromFile(File))
    {
        sf::Sprite TempSprite(*pTempTexture);
        TempSprite.setPosition(x, y);
        pTexture->draw(TempSprite);
        pTexture->display();
        delete pTempTexture;
    }
}

void NsbInterpreter::GLApplyBlur(Drawable* pDrawable, const string& Heaviness)
{
    pDrawable->SetBlur(Heaviness);
}

void NsbInterpreter::GLApplyMask(Drawable* pDrawable, int32_t Time, int32_t Start, int32_t End, int32_t Range, const string& Tempo, string File, bool Wait)
{
    if (sf::Texture* pTexture = LoadTextureFromFile(File))
        pDrawable->SetMask(pTexture, Start, End, Time);
    if (Wait)
        WaitTime = Time;
}

void NsbInterpreter::GLCreateColor(int32_t Priority, int32_t x, int32_t y, int32_t Width, int32_t Height, string Color)
{
    // Workaround
    if (HandleName == "クリア黒")
        return;

    if (Drawable* pDrawable = CacheHolder<Drawable>::Read(HandleName))
    {
        pGame->RemoveDrawable(pDrawable);
        delete pDrawable;
    }

    uint32_t IntColor;

    std::transform(Color.begin(), Color.end(), Color.begin(), ::tolower);
    if (Color[0] == '#')
    {
        Color = string(Color.c_str() + 1);
        std::stringstream ss(Color);
        ss >> std::hex >> IntColor;
    }
    else
    {
        if (Color == "black")
            IntColor = 0;
        else if (Color == "white")
            IntColor = 0xFFFFFF;
        else if (Color == "blue")
            IntColor = 0xFF;
        else if (NsbAssert(false, "Unknown color: %, ", Color))
            return;
    }

    sf::Image ColorImage;
    ColorImage.create(Width, Height, sf::Color(IntColor >> 4, (IntColor >> 2) & 0xFF, IntColor & 0xFF));
    sf::Texture* pTexture = new sf::Texture;
    NsbAssert(pTexture->loadFromImage(ColorImage), "Failed to create color % texture to handle %.", Color, HandleName);
    sf::Sprite* pSprite = new sf::Sprite(*pTexture);
    pSprite->setPosition(x, y);
    CacheHolder<Drawable>::Write(HandleName, new Drawable(pSprite, Priority, DRAWABLE_TEXTURE));
}

void NsbInterpreter::GLLoadMovie(int32_t Priority, int32_t x, int32_t y, bool Loop, bool Alpha, const string& File, bool Audio)
{
    if (Playable* pMovie = CacheHolder<Playable>::Read(HandleName))
    {
        pGame->AddDrawable((Movie*)nullptr);
        delete pMovie;
    }

    string BoxHandle(HandleName, 0, HandleName.find_first_of("/"));
    sf::IntRect* pRect = CacheHolder<sf::IntRect>::Read(BoxHandle);
    Movie* pMovie = new Movie(File, pGame->getSystemHandle(), Priority, Alpha, Audio, pRect);
    pMovie->SetLoop(Loop);

    // Probably unused
    //pMovie->SetPosition(x, y);

    CacheHolder<Playable>::Write(HandleName, pMovie);
    pGame->AddDrawable(pMovie);
}

void NsbInterpreter::GLLoadTexture(int32_t Priority, int32_t x, int32_t y, const string& File)
{
    if (Drawable* pDrawable = CacheHolder<Drawable>::Read(HandleName))
    {
        pGame->RemoveDrawable(pDrawable);
        delete pDrawable;
    }

    sf::Texture* pTexture;

    // Load from texture instead of file
    if (sf::RenderTexture* pRenderTexture = CacheHolder<sf::RenderTexture>::Read(File))
        pTexture = new sf::Texture(pRenderTexture->getTexture()); // TODO: Dont copy
    else
        pTexture = LoadTextureFromFile(File);

    if (!pTexture)
    {
        CacheHolder<Drawable>::Write(HandleName, nullptr);
        return;
    }

    sf::Sprite* pSprite = new sf::Sprite(*pTexture);
    // TODO: Positions are x/y specific!
    if (x < 0 && x >= -SPECIAL_POS_NUM)
        x = SpecialPosTable[-(x + 1)](pTexture->getSize().x);
    if (y < 0 && y >= -SPECIAL_POS_NUM)
        y = SpecialPosTable[-(y + 1)](pTexture->getSize().y);
    pSprite->setPosition(x, y);
    Drawable* pDrawable = new Drawable(pSprite, Priority, DRAWABLE_TEXTURE);
    CacheHolder<Drawable>::Write(HandleName, pDrawable);
    pGame->AddDrawable(pDrawable);
}

void NsbInterpreter::GLParseText(const string& Box, const string& XML)
{
    string NewHandle = Box + "/" + HandleName;
    SetVariable("$SYSTEM_present_text", { "STRING", NewHandle });
    if (Drawable* pText = CacheHolder<Drawable>::Read(NewHandle))
        delete pText;
    Text* pText = new Text(XML);
    CacheHolder<Drawable>::Write(NewHandle, pText);
}

void NsbInterpreter::GLDestroy(Drawable* pDrawable)
{
    if (pDrawable) // Not really needed?
        pGame->RemoveDrawable(pDrawable);
    delete pDrawable;
}

void NsbInterpreter::NSBArrayRead(int32_t Depth)
{
    const string* MemberName = &HandleName;
    ArrayVariable* pVariable = nullptr;

    while (Depth --> 0) // Depth goes to zero; 'cause recursion is too mainstream
    {
        // TODO: check if exists
        ArrayMembers& Members = Arrays[*MemberName].Members;
        for (uint32_t i = 0; i < Members.size(); ++i)
        {
            if (Members[i].first == Params[Params.size() - Depth - 2].Value)
            {
                MemberName = &Members[i].first;
                pVariable = &Members[i].second;
                break;
            }
        }
    }

    if (!pVariable)
        return;

    ArrayParams.push_back(pVariable);
    Params.push_back(*pVariable);
}

void NsbInterpreter::NSBSetState(const string& State)
{
    if (Drawable* pDrawable = CacheHolder<Drawable>::Read(HandleName))
    {
        if (State == "Smoothing")
        {
            if (NsbAssert(pDrawable->Type == DRAWABLE_TEXTURE, "Smoothing non-texture drawable %", HandleName))
                return;

            pGame->GLCallback([pDrawable]()
            {
                sf::Texture* pTexture = const_cast<sf::Texture*>(pDrawable->ToSprite()->getTexture());
                pTexture->setSmooth(true);
            });
        }
    }
    else if (Playable* pPlayable = CacheHolder<Playable>::Read(HandleName))
        if (State == "Play")
            pPlayable->Play();
}

void NsbInterpreter::NSBCreateBox(int32_t unk0, int32_t x, int32_t y, int32_t Width, int32_t Height, bool unk1)
{
    sf::IntRect* pRect = new sf::IntRect(x, y, Width, Height);
    CacheHolder<sf::IntRect>::Write(HandleName, pRect);
}

// TODO: Rename; works for any playable
void NsbInterpreter::NSBGetMovieTime()
{
    Params.clear();
    if (Playable* pPlayable = CacheHolder<Playable>::Read(HandleName))
        Params.push_back({"INT", boost::lexical_cast<string>(pPlayable->GetDuration())});
    else
        std::cout << "Failed to get movie time because there is no Movie " << HandleName << std::endl;
}

void NsbInterpreter::NSBSetOpacity(Drawable* pDrawable, int32_t Time, int32_t Opacity, const string& Tempo, bool Wait)
{
    if (pDrawable->Type == DRAWABLE_TEXT)
    {
        if (NsbAssert(Opacity == 0, "There is no support for text fade out effect"))
            return;

        pGame->GLCallback(std::bind(&Game::ClearText, pGame));
        CacheHolder<Drawable>::Write(HandleName, nullptr); // hack: see Game::ClearText
    }
    else
        pDrawable->SetOpacity(Opacity, Time, FADE_TEX);

    if (Wait)
        WaitTime = Time;
}

void NsbInterpreter::NSBLoadAudio(const string& Type, const string& File)
{
    if (Playable* pMusic = CacheHolder<Playable>::Read(HandleName))
        delete pMusic;

    // TODO: Load from memory? create temp file?
    Playable* pMusic = new Playable(0);
    pMusic->InitAudio();
    CacheHolder<Playable>::Write(HandleName, pMusic);
}

void NsbInterpreter::NSBDisplayText(Text* pText, const string& unk)
{
    if (Playable* pMusic = pText->Voices[0].pMusic)
    {
        pMusic->Play();
        pText->pCurrentMusic = pMusic;
    }
    pGame->SetText(pText);
    Pause();
}

void NsbInterpreter::NSBSetAudioLoop(Playable* pMusic, bool Loop)
{
    pMusic->SetLoop(Loop);
}

void NsbInterpreter::NSBStartAnimation(Drawable* pDrawable, int32_t Time, int32_t x, int32_t y, const string& Tempo, bool Wait)
{
    if (pDrawable->Type == DRAWABLE_TEXT)
    {
        if (!NsbAssert(Time == 0, "There is currently no support for moving text"))
            pDrawable->ToText()->setPosition(x, y);
    }
    else
        pDrawable->AddLerpEffect(LERP_ANIM, x, y, Time);

    if (Wait)
        WaitTime = Time;
}

void NsbInterpreter::NSBSetAudioRange(Playable* pMusic, int32_t Begin, int32_t End)
{
    pMusic->SetRange(Begin, End);
}

void NsbInterpreter::NSBSetAudioState(Playable* pMusic, int32_t NumSeconds, int32_t Volume, const string& Tempo)
{
    pMusic->SetVolume(Volume / 1000.0d);
}

void NsbInterpreter::NSBZoom(Drawable* pDrawable, int32_t Time, float x, float y, const string& Tempo, bool Wait)
{
    pDrawable->AddLerpEffect(LERP_ZOOM, x / 1000.0f, y / 1000.0f, Time);
    if (Wait)
        WaitTime = Time;
}

void NsbInterpreter::NSBSetTextboxAttributes(int32_t unk0, const string& Font, int32_t unk1,
                                             const string& Color1, const string& Color2,
                                             int32_t unk2, const string& unk3)
{
}

void NsbInterpreter::NSBSetFontAttributes(const string& Font, int32_t Size,
                                          const string& Color1, const string& Color2,
                                          int32_t unk0, const string& unk1)
{
}
