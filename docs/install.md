# Instalar Power Dozer v0.8 en Linux

Extraer `power-dozer-v0.8-linux-x86_64.tar.gz` en una carpeta escribible,
conservando las carpetas `lib` y `licenses`:

```bash
sha256sum -c power-dozer-v0.8-linux-x86_64.tar.gz.sha256
tar -xzf power-dozer-v0.8-linux-x86_64.tar.gz
cd power-dozer-v0.8-linux-x86_64
./'Run Power Dozer.sh'
```

El selector solicita una ROM propia de Blast Corps **USA Rev 1**, en formato
z64/v64/n64 o ZIP con una sola ROM válida. SHA-1 normalizado:
`483f7161aea39de8b45c9fbc70a2c3883c4dea8c`. También puede indicarse:

```bash
./'Run Power Dozer.sh' --rom '/ruta/Blast Corps USA Rev 1.zip'
```

No se incluye ROM ni progreso. La ROM instalada, EEPROM/Pak, preferencias y
remapeos se guardan en `data` junto al ejecutable. `--data-dir` permite elegir
otra carpeta escribible. Para conservar el progreso de una instalación anterior,
cerrarla y copiar sus datos hacia una carpeta nueva o vacía:

```bash
./'Run Power Dozer.sh' --data-dir '/ruta/datos-nuevos' \
  --import-data '/ruta/instalacion-anterior/data' --rom '/ruta/Blast Corps USA Rev 1.zip'
```

La importación conserva el origen y no mezcla ni sobrescribe datos existentes.
La ROM se proporciona por separado. Hacer una copia de los datos antes de
cambiar de instalación facilita volver a una versión anterior.

Requiere Linux x86-64, SSSE3/SSE4.1, glibc 2.38 o posterior,
`GLIBCXX_3.4.31`, zlib (`libz.so.1`), controlador Vulkan, escritorio X11/XWayland
y selector de archivos de escritorio. La compatibilidad con otras distribuciones
requiere comprobar el paquete en ellas.

Teclado predeterminado: flechas para mover, Z para acción primaria/confirmar,
X para secundaria/volver, Enter para Start, Shift para frenar/salir,
I/J/K/L para cámara y Q/E para bocina. Alt+Enter alterna pantalla completa.
El menú PC muestra la ayuda, ajustes y remapeo por acción. Los prompts siguen
el último dispositivo activo, con familia Xbox, PlayStation o Switch seleccionable
manualmente. En Switch, las posiciones sur/este/oeste/norte son B/A/Y/X.

Es una beta. La pérdida intermitente de audio sigue en investigación.
Las comprobaciones de mandos físicos, 4K/120 Hz sostenidos y cobertura
amplia de misiones siguen siendo manuales. `BUILD.json` identifica el commit y
los hashes de cada archivo; mantiene `ready_for_release` y
`license_review_complete` en falso. Los avisos están en `licenses`.
La fuente y su método de reconstrucción se describen en [build-source.md](build-source.md).
