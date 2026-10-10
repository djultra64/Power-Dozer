# Reconstruir Power Dozer v0.8

`power-dozer-v0.8-source.tar.gz` contiene el host, scripts de adaptación,
configuración, pruebas y avisos, junto con fuentes fijadas de N64Recomp,
N64ModernRuntime, RT64 y spimdisasm y los tres inputs de mapa/símbolos.
`SOURCES.json` registra repositorios, commits recursivos y SHA-256/tamaño de
cada archivo. El snapshot excluye ROM, overlays, código CPU/RSP generado de
ROM, progreso privado, capturas, evidencia y binarios precompilados.
El fixture de progreso incluido en las pruebas es sintético.

Requisitos de reconstrucción: Linux x86-64, Git/Python, compilador C/C++20,
CMake, Ninja, pkg-config, SDL2 y DBus de desarrollo, headers X11/protocolos Xorg,
zlib, headers del Python usado, venv/pip y conexión para descargar inputs fijados.
Si SDL está fuera de los prefijos estándar, configurar `PKG_CONFIG_PATH` y
`CMAKE_PREFIX_PATH` con su prefijo instalado.

```bash
sha256sum -c power-dozer-v0.8-source.tar.gz.sha256
tar -xzf power-dozer-v0.8-source.tar.gz
cd power-dozer-v0.8-source/host

# Verificar hashes y mostrar la receta sin descargar ni compilar:
python3 tools/bootstrap_sdk.py --sources ../vendor --sdk-root ../sdk --plan

# Preparar herramientas, RT64 y entorno de análisis en un SDK nuevo o vacío:
python3 tools/bootstrap_sdk.py --sources ../vendor --sdk-root ../sdk --jobs 2

# Regenerar y compilar usando una ROM propia USA Rev 1:
python3 tools/build.py '/ruta/Blast Corps USA Rev 1.zip' \
  --sdk-root ../sdk --mapping-source ../mapping --build-dir build
```

Las fuentes exportadas y los inputs DXC/PyYAML/Rabbitizer/base de mandos se
validan antes de usarse. Los transforms del runtime y RT64 producen copias
locales a partir de las fuentes fijadas, sin modificar los originales.
No se necesita acceso al checkout privado usado para preparar el snapshot.
`include/control_atlas_data.h` contiene los sprites ya extraídos: compilar no
requiere los PNG originales del usuario ni ejecutar el extractor del atlas.
Su procedencia y revisión de derechos independiente están en
`config/control-atlas.json` y `NOTICE`.

```bash
ctest --test-dir build --output-on-failure
BC_SDK_ROOT=../sdk python3 -m unittest discover -s tests
```

El ejecutable de desarrollo conserva el nombre de target
`build/blastcorps-prototype`. Los paquetes de distribución lo renombran a
`power-dozer`. El archivo de fuente es determinista para el mismo snapshot;
esto no afirma igualdad bit a bit del ejecutable. La compilación aislada del
método se ha comprobado en checkpoints anteriores; el manifiesto del snapshot
actual conserva sus indicadores de certificación en falso.

`LICENSE` describe el código propio y `NOTICE` delimita dependencias y assets.
La revisión del mapa upstream y del alcance de CPU/RSP generado a partir de la
ROM sigue pendiente. Entregar scripts de regeneración e inputs fijados no
certifica por sí solo una entrega completa de fuente correspondiente GPL.
`license_review_complete`, `corresponding_source_verified`,
`clean_rebuild_verified` y `ready_for_release` permanecen en falso.
