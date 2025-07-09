This is where you put your resources (images, XML files, etc) that you want embedded in your plugin. JUCE will take these, convert them to C++ byte arrays and export a shared library
you can link your plugin against. Resources can then be fetched in code via the `BinaryData` namespace like so:

```cpp
#include "BinaryData.h"

void PluginEditor::loadImages() {
    backgroundImage = juce::ImageCache::getFromMemory(BinaryData::bg_png, BinaryData::bg_pngSize);
}
```

Make sure you add each resource file you want embedded to the `CMakeLists.txt` file:

```cmake
juce_add_binary_data(EmbeddedResources
    SOURCES
        Resources/bg.png
        ...

    HEADER_NAME BinaryData.h
    NAMESPACE BinaryData
)

```