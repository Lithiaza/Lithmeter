#pragma once

struct ImVec4;

namespace SoulMeterTheme {

void Apply(bool preserveUserOverrides = false);

const ImVec4& Accent();
const ImVec4& Live();
const ImVec4& Idle();

}
