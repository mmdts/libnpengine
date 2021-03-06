/* 
 * libnpengine: Nitroplus script interpreter
 * Copyright (C) 2014-2016,2018 Mislav Blažević <krofnica996@gmail.com>
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
#ifndef NSB_CONTEXT_HPP
#define NSB_CONTEXT_HPP

#include "Object.hpp"
#include <stack>
#include <cstdint>

class ScriptFile;
class Line;
class Text;
class NSBContext : public Object
{
    struct StackFrame
    {
        ScriptFile* pScript;
        uint32_t SourceLine;
    };
public:
    NSBContext(const string& Name);
    ~NSBContext();

    bool Call(ScriptFile* pScript, const string& Symbol);
    void Jump(const string& Symbol);
    void Break();
    const string& GetParam(uint32_t Index);
    int GetNumParams();
    const string& GetScriptName();
    ScriptFile* GetScript();
    Line* GetLine();
    uint32_t GetLineNumber();
    uint32_t GetMagic();
    uint32_t Advance();
    void Rewind();
    void Return();
    void PushBreak();
    void PopBreak();
    void WaitText(Text* pText, int32_t Time);
    void WaitAction(Object* pObject, int32_t Time);
    void WaitKey(int32_t Time);
    void Wait(int32_t Time, bool Interrupt = false);
    void Wake();
    void TryWake();
    void OnClick();
    bool IsStarving();
    bool IsSleeping();
    bool IsActive();
    void Start();
    void Request(int32_t State);
    const string& GetName();
    void WriteTrace(ostream& Stream);
    void Update(uint32_t Diff);

private:
    StackFrame* GetFrame();

    Text* pText;
    Object* pObject;
    const string Name;
    uint64_t WaitTime;
    uint64_t Elapsed;
    bool WaitInterrupt;
    bool Active;
    stack<StackFrame> CallStack;
    stack<string> BreakStack;
};

#endif
