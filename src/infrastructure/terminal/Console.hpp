#pragma once

namespace arkan::thanatos::infrastructure::terminal
{
// Detect console UTF-8 capability (best effort).
// コンソールのUTF-8対応を判定（ベストエフォート）。
bool locale_is_utf8();

// Enable Windows VT sequences if possible (no-op on POSIX).
// WindowsのVTシーケンスを可能なら有効化（POSIXでは何もしない）。
void enable_vt_sequences_if_possible();
}  // namespace arkan::thanatos::infrastructure::terminal
