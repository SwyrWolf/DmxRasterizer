## Debug using lldb
For finding a std::expected bad_expected_access in lldb
Bofore running inside lldb:
- `breakpoint set -n abort`
- `target modules list`
After crash
- `bt`
- Look for a frame related to the project such as `netThread.cppm:55`
- `f 10` or `frame select 10` note: 10 is just an example, select relative frame.