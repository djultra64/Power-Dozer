# Fuente privada y reconstrucción con SDK independiente

Preparación local del 10 de octubre de 2026. El archivo de fuentes sirve para
revisar y reconstruir el trabajo sin rutas de Tetrisphere. **No está certificado
como entrega completa de fuente correspondiente GPL ni como distribución con
licencias revisadas.** La reconstrucción independiente del host y sus pruebas CPU están verificadas;
el paquete final de runtime todavía debe actualizarse y aceptarse.
Los bloqueos gráficos/de estabilidad se mantienen independientes de este trabajo.

## Qué contiene

`tools/package_source.py` genera `.local/packages/blastcorps-private-source-review.tar.gz`
y checksum. Incluye host C/C++, headers, configuración, scripts, pruebas,
documentación textual, un fixture de progreso sintético y avisos; fuentes fijadas de N64ModernRuntime,
N64Recomp con sus submódulos recursivos, RT64 con sus contribuciones de fuente,
y spimdisasm; tres archivos de mapa/símbolos que consume el generador.

`SOURCES.json` identifica repositorios, commits recursivos, archivos excluidos,
SHA-256/bytes de cada archivo y estado del checkout del host. Los contenidos
se vuelven a comprobar antes de escribir el tar. Orden, timestamps, propietarios
y encabezado gzip son estables para el mismo snapshot. Esto significa
reproducibilidad del archivo de fuente, no igualdad bit a bit del ejecutable.

Se excluyen `.git`, `.local`, ROM, overlays, código CPU/RSP traducido de ROM,
progreso, Pak, capturas, evidencia binaria, bibliotecas precompiladas, ejecutables
de herramientas, referencia de emulador y variantes de QA. El archivo no depende
del acceso posterior al checkout de Tetrisphere que se usó al producirlo.

Los scripts de adaptación de events/timer/threadqueue/lifecycle/mesgqueue del runtime y del
observador RT64 quedan en el host. Regeneran copias locales de compilación
desde las fuentes fijadas, con sus validaciones de huella; no requieren modificar
el SDK original. Su procedencia, los flags y la receta permanecen revisables.

## Pins y procedencia de los inputs

`config/source-build.json` fija N64Recomp, N64ModernRuntime, RT64 y spimdisasm
(`4b5d4c68eeb9f9eae14b3bb713971433ae2c3c43`, [fuente y licencia MIT](https://github.com/Decompollaborate/spimdisasm/blob/4b5d4c68eeb9f9eae14b3bb713971433ae2c3c43/LICENSE)).
PyYAML queda fijado a 6.0.2 y Rabbitizer Python a 1.16.2, versión inspeccionada en el venv de análisis
de referencia que satisface `spimdisasm/requirements.txt` (>=1.14.0,<2.0.0). Los sdists oficiales de ambos se descargan
por URL exacta y se comprueban por SHA-256/tamaño antes de instalar con pip
`--no-deps`; los avisos MIT están en `licenses/buildtools`. Rabbitizer Python
es un input separado del Rabbitizer C++ incluido en N64Recomp. La metadata
proviene de [PyPI PyYAML 6.0.2](https://pypi.org/pypi/PyYAML/6.0.2/json) y
[PyPI Rabbitizer 1.16.2](https://pypi.org/pypi/rabbitizer/1.16.2/json). La
compilación de extensiones todavía descarga herramientas de build aisladas
(setuptools/wheel, entre otras declaradas por los sdists) sin fijarlas; esto
limita la reproducibilidad bit a bit, aunque los dos sdists están fijados.

Las tres herramientas/bibliotecas DXC para generar shaders se descargan desde
la revisión exacta de dxc-bin y se verifican por SHA-256/tamaño antes de usarlas.
Sus avisos exactos están en `licenses/dxc`. Las fuentes de las herramientas de
análisis/recompilación están presentes; los DXC binarios se tratan como inputs
externos verificados y no forman parte del archivo de fuente. Esa decisión
de inventario no determina el alcance legal de la fuente correspondiente.

La base de mandos se descarga desde su commit fijado y también se valida con
el hash/tamaño ya aceptado. El lock conserva la URL de su licencia/procedencia;
el paquete de runtime conserva su aviso. SDL2/SDL3 y DBus del sistema se
descubren por CMake/pkg-config; para comparar exactamente con el binario actual
hay que fijar además sus fuentes/bottles, compilador, runtime C/C++ y opciones.

## Uso fuera del checkout de desarrollo

Requisitos: Linux x86-64, Git/Python, CMake, Ninja, compilador C/C++20,
pkg-config, headers/librerías SDL2 y DBus, headers X11/protocolos Xorg, zlib, headers de desarrollo Python, soporte venv/pip,
y acceso de descarga para DXC/PyYAML/base de mandos. Para SDL fuera de los
prefijos estándar, configurar `PKG_CONFIG_PATH` y `CMAKE_PREFIX_PATH` con el
prefijo real instalado. El Python que invoque bootstrap necesita sus propios
headers `Python.h`; no basta con otro Python instalado. No hay un prefijo
Homebrew obligatorio en el modo SDK.

Extraer el archivo de fuente en una carpeta nueva. Dentro de
`blastcorps-private-source/host`:

```bash
# Verifica los fuentes exportados y muestra la receta sin descargar/compilar.
python3 tools/bootstrap_sdk.py --sources ../vendor --sdk-root ../sdk --plan

# Construye N64Recomp/RSPRecomp y librerías RT64, y prepara el entorno de análisis.
# El destino SDK debe ser nuevo o vacío; no se mezcla una instalación anterior.
python3 tools/bootstrap_sdk.py --sources ../vendor --sdk-root ../sdk --jobs 2

# Cuando se autorice la reconstrucción del juego, usar una ROM propia válida.
python3 tools/build.py '/ruta/Blast Corps USA Rev 1.zip' \
  --sdk-root ../sdk --mapping-source ../mapping --build-dir build
```

El SDK contiene `deps`, `toolchain/n64recomp-build`, `rt64-build`,
`analysis-venv`, `build-inputs`, `gamecontrollerdb.txt`, `SOURCE-INPUTS.json` y `SDK-BUILD.json`.
La receta compila RT64 estático, SDL/Vulkan y el backend portal de selección.
El CMake original de RT64 no invoca correctamente DXC cuando su ruta contiene
espacios. El bootstrap verifica el SHA completo y un sitio único, conserva
`deps/rt64` y genera `sdk/rt64-source` con una copia local que usa
`cmake -E env`. Registra ambos hashes y el cambio en `RT64-SOURCE-PATCH.json`;
las bibliotecas se compilan desde esa copia. La validación posterior del host
continúa comprobando los fuentes originales, la derivación exacta de la copia
y su metadata, sin ignorar modificaciones. Antes de extraer/regenerar ROM
también valida spimdisasm, DXC y la base de mandos. Al cambiar SDK o volver
al modo legacy, fija la ruta de bibliotecas y limpia el selector contrario
para evitar reutilizar una biblioteca almacenada en la caché de CMake.
Los comandos usan argumentos separados y toleran espacios en los directorios.
El build del host valida fuentes exportados por sus hashes cuando no hay Git.
El modo antiguo `--tetrisphere` conserva su disposición y valores por defecto.

La reconstrucción del juego vuelve a validar la ROM USA Rev 1, extrae overlays,
genera límites/código CPU y microcódigo RSP, crea la copia local de los hooks
y enlaza el host con el SDK. Estos outputs permanecen en `.local` y `build`
del árbol de fuente del destinatario; no se incorporan al archivo de fuente.

## Evidencia y límites de aceptación

El snapshot posterior a lifecycle y mesgqueue de 10.309 archivos
se extrajo en `/tmp` con espacios
y todos sus hashes pasaron. N64Recomp y RSPRecomp compilaron desde las fuentes
exportadas (salida 0). RT64 estático y sus cuatro bibliotecas dependientes
(re-spirv, nfd, zstd, plume) también compilaron (salida 0), con SDL/DBus/X11
descubiertos por pkg-config. La ejecución real de shaders DXC confirmó el fix
de rutas con espacios; el build expuso además la necesidad de headers X11.
El venv independiente contiene PyYAML y Rabbitizer fijados. Esta comprobación
por etapas no es todavía una ejecución end-to-end del comando bootstrap nuevo.
La reconstrucción completa del host exportado terminó 0: generación CPU/RSP,
configuración, los 224 targets y enlace. Se usó la ROM local como input de sólo
lectura y todas las escrituras ocurrieron en el host temporal. CTest CPU pasó
41/41 y Python 110/110 con ROM propia, sin skips, tras integrar el transform
mesgqueue que valida el source SDK original y genera una copia local. Los
fixtures normal/timed prueban que retries críticos retenidos no ocultan VI
ni sobrescriben colas llenas, y conservan las reglas jam/nonretry. Configure y
build del export terminaron 0; SDK, herramientas y ROM original no se modificaron.
ELF privado de este checkpoint SHA-256
`27d46626bbd6b3e48a0317b4e20191bcb2a68d9c949862289470d1bdfe1c23ad`.
La reconstrucción previa lifecycle había pasado 39/107; el binario previo
`537380aa960c9b52357542726d348ee5b070be06c468b085f486340413f12512`
corresponde a ese estado anterior. Los resultados/checksums están en
`.local/qa/source-host-build.json`.

La prueba ROM permite `BC_TEST_ROM`, luego `.local/rom.z64`, luego un ZIP local; si no hay
ninguno, sólo esa comprobación privada se omite. La prueba de ROM corrupta
sigue ejecutándose sin datos privados. El fixture `legacy-completed.hex` es
sintético y se incluye por su ruta exacta, sin permitir otros hex de progreso.

Los primeros intentos expusieron Rabbitizer ausente, headers Python ausentes,
compiler sysconfig gcc-13 inexistente y un fixture sintético omitido. La receta
resuelve esos requisitos y conserva los resultados finales en `.local/qa`.
El checkpoint CPU aquí registrado no certifica su ejecución GPU ni
correspondencia con el objeto del paquete final. Reconstrucción CPU no equivale a equivalencia binaria,
instalación/runtime ni certificación de fuente correspondiente.

No se modificó el binario de producción ni se ejecutó el juego en este trabajo
de fuentes. El checksum final debe registrarse después de incluir todos los
cambios y documentación, y la prueba del paquete final debe usar ese estado.

## Pendientes que impiden certificar fuente correspondiente/licencias

1. El mapa `game.us.v11.yaml` y los dos `symbol_addrs` proceden de SlaveOfIDO
   commit `1ab6616edb6c16b642c229ab6d55c3c8687c7945`. Se conservan sus hashes
   y origen. No se encontró una declaración de licencia en la raíz revisada;
   esto no permite concluir propiedad o permiso de redistribución.
2. CPU/RSP traducidos proceden de instrucciones de la ROM suministrada por
   el usuario. Un generador MIT no asigna automáticamente su licencia al juego
   traducido. Este archivo sólo entrega el método de regeneración; hay que
   resolver si constituye la fuente correspondiente completa del ejecutable
   entregado y el alcance de derechos del contenido generado.
3. No hay declaración de licencia propia del port en la raíz. Elegir/aplicar
   los avisos y mecanismo de entrega adecuados requiere establecer el alcance
   de los componentes cubiertos y las fuentes que pueden entregarse.
4. Completar el inventario de bibliotecas/headers/shaders realmente incorporados,
   con sus avisos, y fijar/entregar o documentar todos los inputs que correspondan.
   Los commits y textos presentes no sustituyen la revisión de esa combinación.
5. Ejecutar la reconstrucción completa con una ROM propia en un árbol aislado,
   comparar la identidad/código y flags de sus binarios con el paquete final,
   verificar instalación/runtime, y definir el acceso a las fuentes para cada
   destinatario del código objeto conforme a la [GPL v3 del runtime](https://github.com/N64Recomp/N64ModernRuntime/blob/cdf5abbd5026fef5c364c676e4667c45e42b6863/COPYING).

Mantener `license_review_complete`, `corresponding_source_verified` y
`clean_rebuild_verified` en falso hasta cerrar estas evidencias. Las pruebas
manuales de 120 fps y cobertura de misiones no resuelven estos pendientes.


Aceptación nativa posterior: el ELF reconstruido SHA-256
`537380aa960c9b52357542726d348ee5b070be06c468b085f486340413f12512`
completó 180 segundos fuera del checkout, en una carpeta con espacios:
10.801 VI, 10.948 tareas gráficas y 5.397 tareas de audio, movimiento,
pausa/Continue y Restart. Cargó ALF y el progreso completado desde copias
privadas; los seeds quedaron intactos. La salida virtual usó radv,
1080p, 16:9 y objetivo configurado 120, sin exigir el umbral de fps.
No es aceptación de salida física/audio audible ni del defecto gráfico pendiente.

The updated mesgqueue-fairness host passed a separate native regression in
`gameplay-3ar19xmo`, using ELF
`27d46626bbd6b3e48a0317b4e20191bcb2a68d9c949862289470d1bdfe1c23ad`
and cwd outside the checkout. It completed 180 seconds at VI 10801, graphics
10951 and audio 5399, with movement, pause/Continue, Restart and existing
completed-level replay verified. EEPROM/Pak seed sources stayed unchanged.
This is runtime acceptance of the rebuilt host in the checked flow; no binary
reproducibility, comprehensive platform coverage or source-license certification
is inferred. The final packaged object still needs its own acceptance.
