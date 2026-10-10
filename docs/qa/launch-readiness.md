# Preparación del lanzamiento privado de Linux

Actualización posterior: los defectos concretos de HUD, ticker, sprites del mapa, cielo y fundido reportados en los videos reabren ese alcance gráfico; la aceptación previa del terreno se conserva. El ejecutable de desarrollo ya incorpora las correcciones y un guard de despacho de audio. Véase [el seguimiento y sus límites](graphics-audio-followup-2026-10-10.md). Los paquetes citados abajo conservan el checkpoint anterior.

Revisión del 10 de octubre de 2026. Este documento prepara una beta privada
revisable; no certifica una candidata ni autoriza publicación. El alcance
actual entrega una beta local con los gráficos actuales aceptados por el usuario
y la limitación de audio indicada abajo. Los 120 fps sostenidos y
la cobertura amplia de misiones se registran en [manual-testing.md](manual-testing.md).

## Artefactos de la beta local

El runtime `.local/packages/blastcorps-private-linux-x86_64.tar.gz` y la fuente
`.local/packages/blastcorps-private-source-review.tar.gz` se generaron desde el
commit limpio `99bb524bd702fa8a54350ff7a2aa18afd6b637a3`:

```text
Runtime SHA-256 0b6824931902aafcdb679540c7bc0b87b57275cf069a1308654f40b366dbdfa2
ELF instalado 7fb9d803ddb1bd3cea11766ca7f705bfb51390e6440deaeb1bc6311b95349568
91 miembros; 90 archivos en el inventario; 77 textos de licencia
Fuente SHA-256 b06a3394b05ee00eb076b58fb20042af54c402dc9fc954815984559b6bc398d0
10.309 archivos de fuente verificados
ready_for_release=false; revisión completa de licencias pendiente
```

Todos los hashes se verificaron; el runtime se extrajo en una ruta nueva con
espacios fuera del checkout. No contienen ROM, progreso privado ni capturas.
Las dependencias inspeccionadas con `ldd` resuelven y el ELF usa `$ORIGIN/lib`.
La evidencia está en `.local/qa/package-checkpoint-current.json`.

El mismo ELF instalado pasó sesiones de 180 y 360 segundos con movimiento,
pausa y Restart (`gameplay-ghunmacy` y `gameplay-jck7jpdz`). Sin cambios de
código, una sesión anterior (`gameplay-j8o4si3t`) dejó de procesar audio mientras
los gráficos avanzaban. **El bloqueo intermitente de audio sigue sin resolver**;
las sesiones posteriores no demuestran su eliminación. Se entrega como beta
local con esta limitación conocida. Las instrucciones breves están en
[private-beta-notes.md](private-beta-notes.md).

Los archivos conservan el checkpoint 99bb524; estas notas posteriores no están
dentro del snapshot. No se regeneran ni se modifica el binario para esta entrega.

## Requisitos comprobables de este binario

Linux x86-64 con SSSE3 y SSE4.1, controlador Vulkan operativo, escritorio
X11/Wayland y una carpeta de datos escribible. La inspección estática del ELF
del paquete actual requiere símbolos de **glibc 2.38** y
**GLIBCXX_3.4.31** en `libstdc++.so.6`. Es un requisito observado de símbolos,
no una certificación de distribuciones o de todas las CPU compatibles.
Una compilación posterior debe repetir esta inspección.

Se incluyen `libSDL2-2.0.so.0` (sdl2-compat 2.32.70), `libSDL3.so.0`
(3.4.12), `libdbus-1.so.3` (1.16.2, bottle 1.16.2_1), `libdxcompiler.so`
y `libdxil.so`. El ejecutable instalado usa `$ORIGIN/lib`; las bibliotecas
relocadas usan `$ORIGIN` cuando tenían RPATH. Permanecen dependencias del
sistema: libc, libm, libgcc, libstdc++, el cargador ELF y **libz.so.1** para
DXC. Los backends de pantalla/audio y Vulkan pueden cargar otras bibliotecas
en ejecución; `DT_NEEDED` no constituye su inventario completo.
Windows y otras distribuciones todavía no tienen aceptación nativa.

## Instalación y notas para quien pruebe la beta

1. Comprobar el `.sha256` y extraer el archivo en una carpeta escribible.
2. Ejecutar `./'Run Blast Corps.sh'` desde la carpeta extraída. El selector
   solicita una ROM propia sin modificar de Blast Corps USA Rev 1. También
   puede usarse `./'Run Blast Corps.sh' --rom '/ruta/Blast Corps.zip'`.
3. Se aceptan z64/v64/n64 o ZIP con una única ROM válida. La imagen normalizada
   debe tener SHA-1 `483f7161aea39de8b45c9fbc70a2c3883c4dea8c`.
4. La ROM validada queda en `data/cache/rom.z64`; los overlays se regeneran en
   memoria. EEPROM, Pak, preferencias y remapeos viven en `data` junto al
   ejecutable. `--data-dir PATH` permite elegir otra carpeta.
5. Para conservar una instalación anterior, cerrar el juego y usar
   `--import-data '/ruta/instalacion-anterior/data'` hacia una carpeta nueva
   o vacía. La importación copia progreso/preferencias; no mezcla una
   instalación existente. El origen permanece intacto. Importar la ROM
   por separado. Conservar la instalación anterior hasta verificar la nueva.

La beta ofrece opciones PC en el estilo original, cambio de pantalla con
Alt+Enter, remapeo por acción, ayuda e instrucciones según el último dispositivo
activo y selección manual de familia cuando la detección es ambigua. Hay
evidencia automatizada de Simian Acres, pausa, Continue, Restart, fallo,
objetivo completado y recarga del progreso, además de mandos SDL virtuales.
Estos resultados no abarcan todas las misiones ni mandos físicos.

El objetivo de interpolación a 120 se midió en 720p/1080p sobre una salida
virtual de 120 Hz. La medición sostenida de 4K quedó alrededor de 116,5 fps.
La pantalla física, continuidad audible, cobertura amplia y rendimiento
sostenido quedan pendientes de la lista manual. El usuario acepta los gráficos
actuales para esta beta local. El bloqueo del productor externo ya tiene
corrección y regresiones nativas; el audio intermitente y la revisión de
fuentes/licencias siguen abiertos. Su estado está en [readiness.md](readiness.md).

## Inventario de avisos y procedencia

La licencia se obtiene del texto inspeccionado de cada fuente, no de la ausencia
de metadatos. Los paquetes que aparecen en el escaneo recursivo pueden contener
componentes de herramientas, ejemplos o Windows que no se enlazan en Linux;
el número de avisos no demuestra una revisión completa.

| Componente real o categoría | Evidencia primaria inspeccionada | Estado y siguiente paso |
| --- | --- | --- |
| N64ModernRuntime enlazado estáticamente y archivos librecomp compilados | [COPYING fijado](https://github.com/N64Recomp/N64ModernRuntime/blob/cdf5abbd5026fef5c364c676e4667c45e42b6863/COPYING), GPL v3; `CMakeLists.txt` de este port enlaza `ultramodern` y compila librecomp | Texto incluido. Preparar la fuente correspondiente y los avisos de cambios antes de distribuir el ejecutable. |
| N64Recomp / RSPRecomp | [LICENSE fijada](https://github.com/N64Recomp/N64Recomp/blob/ffb39cdad1da5de07eaaa48bd1db4a89a7986771/LICENSE), MIT | Aviso incluido. Distinguir herramientas de generación de código y código/headers incorporados. |
| RT64 | [LICENSE fijada](https://github.com/rt64/rt64/blob/43373749dac9bbc1b653e6a02aed40a9e1783bed/LICENSE), MIT | Aviso incluido. La licencia de RT64 no sustituye la de sus contribuciones. |
| re-spirv, nativefiledialog, zstd, plume, ImGui, im3d, implot, hlslpp, ddspp, xxHash, stb y miniz del runtime | Fuentes fijadas por los árboles/submódulos anteriores y bibliotecas estáticas enumeradas por CMake | Avisos con nombre LICENSE/COPYING ya incluidos; hay que vincular cada biblioteca/header/shader usado a sus textos y revisiones. |
| DXC y DXIL redistribuidos | [dxc-bin fijado](https://github.com/rt64/dxc-bin/tree/cc15e715ee378a4f675b335bd1071ff105873fc8), versión local `1.8(dev;4461-11e1318c)`; [LICENSE.TXT exacta](https://github.com/microsoft/DirectXShaderCompiler/blob/11e1318c3ed77aa08326e16eaf52598ad6430546/LICENSE.TXT) y [ThirdPartyNotices exactos](https://github.com/microsoft/DirectXShaderCompiler/blob/11e1318c3ed77aa08326e16eaf52598ad6430546/ThirdPartyNotices.txt) | Se agregan ambos textos con SHA/bytes e inputs binarios verificados en `licenses/dxc`. La versión identifica el compilador; DXIL acompaña la misma revisión binaria, sin atestación independiente de su compilación. |
| JSON 3.12.0 incorporado por runtime/RT64 | Headers exactos con SPDX; [MIT oficial fijada](https://github.com/nlohmann/json/blob/55f93686c01528224f448c19128836e7df245f72/LICENSE.MIT) | Se agrega el texto MIT y la atribución SPDX de Niels Lohmann, Hedley, Abseil, Hoehrmann y Loitsch, con hashes de ambos headers. |
| ConcurrentQueue / BlockingConcurrentQueue y semaphore | Avisos BSD y zlib dentro de los tres headers de N64ModernRuntime fijado | Se agregan avisos exactos de Cameron Desrochers y Jeff Preshing en `licenses/embedded`, sin elegir una licencia alternativa ausente. |
| miniz compilado dentro de RT64 | `src/contrib/miniz/miniz.c` enumerado en CMake de RT64 | Se agregan los tres bloques de aviso distintos, extraídos sin editar los textos MIT/Unlicense. La licencia del miniz del runtime no sustituye estos avisos. |
| Vulkan-Headers / SPIRV-Cross | Sus `LICENSES/*.txt`, antes omitidos por el filtro de nombres | El escaneo ahora incluye los textos Apache/MIT/otros dentro de LICENSES y variantes NOTICE/ThirdPartyNotices. Se conserva la atribución del árbol fijado. |
| SDL2-compat y SDL3 | Textos Zlib instalados, SBOM/receipts Homebrew y versiones anteriores | Textos incluidos. Guardar URLs/checksums de las fuentes y origen de los bottles en el inventario final; no están completos en BUILD.json. |
| DBus 1.16.2 | [archivo fuente oficial](https://dbus.freedesktop.org/releases/dbus/dbus-1.16.2.tar.xz), COPYING y LICENSES verificados en `licenses/dbus/provenance.json` | Textos AFL 2.1, GPL 2 o posterior y MIT incluidos. Documentar la ruta elegida para la distribución y cumplirla; incluir ambos textos no resuelve esa revisión. |
| SDL_GameControllerDB | [commit fijado](https://github.com/mdqinc/SDL_GameControllerDB/tree/c1d5289a1f713b30a2c121e9fe6529d39360be0f), licencia Zlib, hashes de base/texto | Aviso y procedencia incluidos; no actualizar la base durante la aceptación final. |
| Fuente propia, metadatos de decompilación y código generado de ROM | Fuente de este checkout y mapa [SlaveOfIDO fijado](https://github.com/SlaveOfIDO/blastcorps/tree/1ab6616edb6c16b642c229ab6d55c3c8687c7945) | No hay una declaración de licencia del port en la raíz ni una licencia del mapa en la raíz inspeccionada. Eso no determina titularidad ni derechos sobre el juego; resolver alcance y procedencia antes de publicar o ofrecer fuente. |

Los manifests nuevos conservan `complete_dependency_license_review=false`.
Los bytes originales del SDK y las bibliotecas se verifican antes de copiar
los avisos: una dependencia cambiada exige revisar/actualizar su procedencia,
no reutilizar automáticamente un texto obtenido de otra versión.

## Fuente correspondiente y reconstrucción pendientes

La [GPL v3, secciones 1, 4–6](https://github.com/N64Recomp/N64ModernRuntime/blob/cdf5abbd5026fef5c364c676e4667c45e42b6863/COPYING)
define la fuente correspondiente y las condiciones de entregar código objeto.
El paquete de runtime incluye el texto GPL y hay un snapshot separado de
fuentes revisables. Ese snapshot todavía no está certificado como fuente
correspondiente ni tiene una vía de entrega pública documentada.
La preparación concreta debe conservar fuentes/headers enlazados, scripts de
generación/instalación, interfaces y cambios del runtime/RT64, indicar qué
archivos se modificaron y establecer el mecanismo de entrega de fuente
aplicable. Los pins de repositorios por sí solos no cumplen ese trabajo.

El binario de runtime se compiló con la estructura privada de
Tetrisphere y bibliotecas precompiladas. Ahora `tools/package_source.py`
exporta un snapshot privado revisable con fuentes recursivas, mapa, avisos,
patches/scripts y hashes; `tools/bootstrap_sdk.py` construye un SDK separado,
y `build.py --sdk-root` permite reconstruir desde el host exportado con ROM
propia, sin depender del checkout de Tetrisphere. La receta y los límites están
en [source-rebuild.md](source-rebuild.md). N64Recomp/RSPRecomp y RT64/plume/zstd/
re-spirv/nfd ya compilaron desde ese export en rutas con espacios. Las
verificaciones incluyen spimdisasm, DXC y la base de mandos, además de validar
la copia de RT64 preparada y limpiar la selección de bibliotecas al cambiar SDK.
El host aislado ya completó generación, compilación y enlace (224 targets),
con CTest CPU 41/41 y Python 110/110 tras añadir el transform mesgqueue
y sus fixtures normal/timed. Configure/build del SDK/export terminaron 0; el
checkpoint está registrado en source-host-build.json. La evidencia se registra por separado; esto no certifica
por sí solo fuente correspondiente ni licencias, equivalencia con el objeto
entregado o runtime GPU. Aún deben fijarse
las herramientas Python de build transitivas, bottles/runtime C/C++ y el
mecanismo de acceso a fuentes que corresponda al binario final.

El host exportado con la corrección mesgqueue, ELF
`27d46626bbd6b3e48a0317b4e20191bcb2a68d9c949862289470d1bdfe1c23ad`,
pasó sus suites CPU y 180 segundos nativos fuera del checkout. La receta e
inputs están en [source-rebuild.md](source-rebuild.md). El snapshot incluye
los scripts de transformación del SDK, fuentes, mapa, configuración y avisos;
requiere la ROM propia para regenerar CPU/RSP. Estos resultados técnicos no
certifican la revisión de licencias ni la fuente correspondiente del paquete.

Antes de distribuir:

- Revisar el snapshot de fuentes/scripts y completar el inventario de todos los inputs
  (commits recursivos, patches locales, versiones/opciones de compilación,
  checksums de bibliotecas precompiladas y herramientas, paquetes Python).
- Probar desde un directorio limpio la reconstrucción de RT64/plume/zstd/
  re-spirv/nfd, N64Recomp/RSPRecomp y el port. No depender del checkout privado
  de Tetrisphere ni de archivos generados no reconstruibles.
- Definir cómo el destinatario genera las partes que usan su ROM válida
  sin empaquetar ROM, overlays, progreso o capturas. No considerar resuelta
  la licencia/redistribución del código generado por haber omitido la ROM.
- Establecer avisos del port y procedencia del mapa/código incorporado con
  base en evidencia. Evitar atribuir propiedad o licencias por inferencia.
- Asociar el paquete de fuente y el mecanismo de acceso al checksum exacto
  del binario final. La revisión completa de dependencias sigue pendiente.

## Receta de empaquetado para un cambio posterior

La entrega actual ya está empaquetada; no necesita recompilarse ni más pruebas
para abrirla localmente. Si cambia el código, cerrar las pruebas GPU que usen
el ejecutable, compilar el nuevo estado y comprobar el paquete actualizado:

```bash
python3 tools/package_linux.py --build build \
  --tetrisphere '/var/home/alfredo/Documents/ChatGPT/Tetrisphere/.worktrees/options-beta' \
  --output '.local/packages/blastcorps-private-linux-x86_64.tar.gz'
sha256sum -c .local/packages/blastcorps-private-linux-x86_64.tar.gz.sha256
```

El empaquetador instala una copia con strip; no altera el ELF de desarrollo.
Inspeccionar BUILD.json, hashes, RPATH, símbolos requeridos y miembros del tar.
Extraer en una carpeta nueva con espacios fuera del checkout, usar datos
aislados y aceptar arranque frío, selección/importación de ROM, misión,
guardar/salir/recargar, pausa/Restart y opciones/controles del paquete.
Conservar logs privados y hashes del progreso original antes/después.

Excluir ROM, overlays extraídos, datos del usuario, capturas, logs privados,
emulador de referencia y variantes de QA. El inventario no debe incluir
`data`, `.local` ni archivos de extensión privada. La cantidad de avisos
y el éxito de las suites no resuelven el audio intermitente ni certifican
fuentes/licencias. Mantener ambos flags de
release en falso hasta cerrar sus evidencias por separado.
