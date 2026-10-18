# XII developer documentation

- [Build, test, and troubleshoot](Building.md)
- [CI, security, releases, and maintainer settings](Infrastructure.md)
- [Contribution guide](../CONTRIBUTING.md)
- [World rendering migration](Rendering/WorldRenderingMigration.md)
- [Renderer remaining work](Rendering/RemainingWork.md)
- [Editor modernization](EditorModernization.md)
- [Inspector rendering](InspectorRendering.md)

## Generating the API docs with Doxygen

## Install Doxygen and Graphviz Dot Tool

* Install Doxygen: <https://www.doxygen.nl/download.html>

* Install the Graphviz Dot Tool: <http://www.graphviz.org/Download.php>

## Generate Docs

* Use the *Doxyfile* in this directory.
* Set `%XII%/Documentation` as the working directory.

Either run it with the *Doxywizard*, or from the command line:

```cmd
%XII%\Documentation>"C:\Program Files\doxygen\bin\doxygen.exe" Doxyfile
```

The output is written to: `%XII%/Output/Doxygen/html/index.html`.
The manual `update-api-docs.yml` workflow also generates a downloadable HTML artifact.
Cross-repository publication requires the protected settings in [Infrastructure.md](Infrastructure.md).

## Searchdata.xml

If you need to generate a searchdata.xml file, check this documentation:
<https://www.doxygen.nl/manual/extsearch.html>

You might need to run Doxygen twice, once with the settings to generate the `searchdata.xml` file, once to create the actual output with search enabled.

