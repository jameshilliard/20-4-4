/*
 * tivoCallGraph.h
 *
 * This file contains declarations for TiVo's extensions
 * to gcc for generating call graph information.
 *
 */

#ifndef __TIVO_CALL_GRAPH_H__
#define __TIVO_CALL_GRAPH_H__

void tivoCallGraphClose PARAMS ((void));
const char* tivoGetId PARAMS ((tree));
void tivoReportFunctionCall PARAMS ((tree));
void tivoGenCallGraphForGlobal PARAMS ((void));
void tivoReportAddressOf PARAMS ((tree));

#endif /* __TIVO_CALL_GRAPH_H__ */
