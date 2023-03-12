# XII

XII is an advanced game engine written in C++. It is currently mainly developed on Windows, and higher level functionality such as rendering and the tools are only available there, but the core libraries are also available for other platforms such as Mac and Linux.

XII is built in a modular way, enabling users to either use all available functionality, or to pick and choose individual features and build the rest yourself. Larger features are implemented through engine and editor plugins and can therefore be easily removed or replaced. For instance particle effects are all provided through runtime plugins.

> **Note:**
>
> Please be aware that features such as rendering and the editor are only available on Windows-x64. On all other platforms we only compile and test the core engine functionality.