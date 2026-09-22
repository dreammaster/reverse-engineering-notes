#ifndef YENDOR23_DOCUMENT_STDIO_H
#define YENDOR23_DOCUMENT_STDIO_H

#include "document.h"

/* Reads just the document-text region out of a WORLD.DAT file; for tools and tests. */
bool documentCatalogReadWorldDatFile(DocumentCatalog *catalog, GameKind game, const char *path);

#endif
