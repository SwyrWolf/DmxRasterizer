## Console print enumuration
This only works in late version of Window 10 and above.
```cpp
std::cout << "Received Data:\n";
auto deltas = logger.GetTimeDeltasMs();

for (auto [index, value] : deltas | std::views::enumerate) {
	std::cout << "\r\033[K";
	std::cout << "-> Universe " << index << ": " << value << "ms\n";
} 

std::cout << "\033[" << (deltas.size() + 1) << "A" << std::flush;
```

Modern C++ way of normalizing a range
```cpp
for (auto&& [src, dst] : std::views::zip(DmxTexture.DmxData, DmxTexture.ChannelsNormalized)) {
	dst = as<f32>(src) / 255.0f;
}
```