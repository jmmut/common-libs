
/**
 * @file graph.h
 *
 * Generic graph data structure with handling and algorithms functions.
 *
 * In some functions you can choose how to identificate a vertex.@n
 * _s: char * name  @n
 * _i: int id  @n
 * _v: vertex_t * pointer  @n
 */
/*
 * Podemos dar la opcion a cambiar el tipo de grafo despues de crearlo,
 * es decir, si se permite el cambio automatico/manual de modo de grafo "si" "no" "depende"
 * 
 * 
 * //typedef int vertex_id; ??
 *
 * 
 * TODO:
 * 		Darle coherencia
 * 		Correcto sincronismo (sync_mode)
 * 
 * DUDAS:
 */

#ifndef _GRAPH_H_
#define _GRAPH_H_

#include <float.h>
#include "linked_list.h"
#include "array_list.h"
#include "containers.h"
#include "khash.h"
#include "heap.h"
#include <omp.h>

KHASH_MAP_INIT_STR(gr,int)
KHASH_MAP_INIT_INT(ii,int)


typedef struct edge {
    int src_id;
    int dst_id;
    void *data;
    float weight;
    //char* name;
    
} edge_t;

typedef int vertex_id_t;

typedef struct vertex {
    void *data;
    linked_list_t *src;
    linked_list_t *dst;
    linked_list_t *nd;
    char* name;
    vertex_id_t id;
} vertex_t;

typedef struct graph_stats{
    char valid;
    
    int *grade;
    float mean_grade;
    
    int * betweenness;
    int *max_jumps; 
    float *max_w;
    
    int *non_reach;
    
    int *non_isolated;
    
    int   max_jumps_path;
    float max_w_path;
} graph_stats_t;

typedef struct subgraph {
    linked_list_t *vertices;
    
    int num_vertices;	//()
    graph_stats_t stats;
    
}subgraph_t;

typedef struct graph {
    khash_t(gr) *dict;
    array_list_t *vertices;
    linked_list_t *removed_vertices;
    int num_vertices;	//()
    int num_edges;		//()
    
    short int directed;	//si, no y mixto
    short int cyclic;
    short int multiple;
    short int strict;
    short int non_negative;
    //reemplazar todo por una mascara?
    
    graph_stats_t stats;
    int chunk;
    int sync_mode;
} graph_t;


typedef struct graph_path{
    float distance;
    int father;
} graph_path_t;

#define GRAPH_MAX_NAME_LENGTH 256

/// Wether an edge allows leaving a vertex, or reaching a vertex.
enum EdgeDirection {GRAPH_EDGE_IN = 1, GRAPH_EDGE_OUT = 2, GRAPH_EDGE_IN_OUT = 3};

/// Wether an edge has directionality or not.
enum EdgeType {GRAPH_EDGE_DIRECTED = 1, GRAPH_EDGE_NON_DIRECTED = 2, GRAPH_EDGE_ALL = 3};

/// Features of the graph, expressed in a mask.
enum GraphType {
    GRAPH_DIRECTED			= 0b00000001,
    GRAPH_NON_DIRECTED				= 0b00000010,
    GRAPH_MIXED_DIRECTED		= 0b00000011,
    GRAPH_CYCLIC				= 0b00000000,
    GRAPH_ACYCLIC				= 0b00000100,
    GRAPH_STRICT				= 0b00001000,
    GRAPH_NON_NEGATIVE_WEIGHT	= 0b00010000,
    GRAPH_UNITARY_WEIGHT    	= 0b00100000    //In run_stats don't need to count jumps 
    //GRAPH_MULTIPLE			0b00010000
};

///Plot Types
enum Plot_Type {PLOT_GRADE, PLOT_BETWEENNESS, PLOT_JUMPS, PLOT_NON_REACHABLE, PLOT_WEIGHT};

/***********************    Creation and initialization    ****************************/
/**
 * @param mask Type of the graph. You can specify several features by |'ing them e.g.
 * graph_new (GRAPH_NON_DIRECTED | GRAPH_ACYCLIC | GRAPH_NON_NEGATIVE_WEIGHT, ...
 *
 * @param initial_num_vertices First estimation of how many vertices the graph will have.
 * A good estimation will result in fewer reallocs. Note that the graph is always created empty, this 
 * parameter doesn't add random vertices.
 *
 * @param SYNC_MODE Wether the graph must use mutex or not.
 */
graph_t* graph_new(enum GraphType mask, int initial_num_vertices, int SYNC_MODE);

/**
 * By the moment, 1 is the unique valid value for m.
 * @param order Number of vertices the graph returned will have.
 * @param m Edges added each step.
 */
graph_t *graph_create_free_scale (enum GraphType mask, int order, int SYNC_MODE, int m);

graph_t *graph_copy_vertices (graph_t * graph_p);

/***********************    Destruction    ****************************/

int graph_free(void (*vertex_data_callback) (void* vertex_data), void (*edge_data_callback) (void* edge_data), graph_t*);

int graph_clear(void (*vertex_data_callback) (void* vertex_data), void (*edge_data_callback) (void* edge_data), graph_t*);

/***********************    Vertex Functions    ****************************/
int graph_add_vertex(char* name, void* vertex_data, graph_t*);

int graph_find_vertex(char* name, graph_t*);	// assumes only one ocurrence

int graph_exists_vertex_s(char* name, graph_t*);
int graph_exists_vertex_i(int id, graph_t*);
//contains??

int graph_reachable_vertex(int src, int dst, graph_t*);

vertex_t* graph_get_vertex_s(char* vertex_name, graph_t * graph_p);
vertex_t* graph_get_vertex_i(int vertex_id, graph_t * graph_p);

void* graph_get_vertex_data_s(char* name, graph_t*);
void* graph_get_vertex_data_i(int id, graph_t*);


/**
 * Returns a linked_list with all the vertices reachables in k_jumps or less from a given vertex.
 *
 * @param edge_type Wether you want to list vertices reachables (childs or destinations), or vertices with
 * reverse direction (fathers or sources).
 */
linked_list_t* graph_get_vertex_neighborhood_v(vertex_t* vertex_p, enum EdgeDirection edge_dir, int k_jumps, graph_t*);
linked_list_t* graph_get_vertex_neighborhood_s(char* name, enum EdgeDirection edge_dir, int k_jumps, graph_t*);
linked_list_t* graph_get_vertex_neighborhood_i(int vertex_id, enum EdgeDirection edge_dir, int k_jumps, graph_t*);

linked_list_t* graph_get_vertex_adjacents_v(vertex_t* vertex_p, graph_t*);
linked_list_t* graph_get_vertex_adjacents_s(char* name, graph_t*);
linked_list_t* graph_get_vertex_adjacents_i(int id, graph_t*);


int graph_remove_vertex_s(char* vertex_name, void (*vertex_data_callback) (void* vertex_data),void (*edge_data_callback) (void* edge_data), graph_t*);
int graph_remove_vertex_i(int vertex_id, void (*vertex_data_callback) (void* vertex_data),void (*edge_data_callback) (void* edge_data), graph_t*);


/***********************    Edge Functions    ****************************/
/**
 * If you want an edge to have an explicit weight, use the _w functions. 
 * Otherwise it will be set to 1
 */
int graph_add_edge_s(char* src, char* dst, void* edge_data, enum EdgeType edge_type, graph_t*);
int graph_add_edge_i(int src, int dst, void* edge_data, enum EdgeType edge_type, graph_t*);
int graph_add_edge_sw(char* src, char* dst, void* edge_data, enum EdgeType edge_type, float weight, graph_t*);
int graph_add_edge_iw(int src, int dst, void* edge_data, enum EdgeType edge_type, float weight, graph_t*);

int graph_remove_edge_s(char* src, char* dst, enum EdgeType edge_type, void (*edge_data_callback) (void* edge_data), graph_t*);
int graph_remove_edge_i(int src, int dst, enum EdgeType edge_type, void (*edge_data_callback) (void* edge_data), graph_t*);
int graph_remove_edge_e(edge_t *edge_p, enum EdgeType edge_type, void (*edge_data_callback) (void* edge_data), graph_t*);

edge_t* graph_get_edge_s(char* src, char* dst, enum EdgeType edge_type, graph_t*);
edge_t* graph_get_edge_i(int src, int dst, enum EdgeType edge_type, graph_t*);
edge_t* graph_get_edge_v(vertex_t* v_src, vertex_t* v_dst, enum EdgeType edge_type, graph_t*);

/***********************    Path Functions    ****************************/
//linked_list_t* graph_path_get(int src, int dst, graph_t*);

/***********************    Others    ****************************/
void graph_print(graph_t*);	
void graph_print_dot(char* file_name, graph_t*);
void graph_print_dot_w(char* file_name, graph_t*);
//void graph_read_dot(char *file_name, graph_t*);
int graph_get_order (graph_t*);	// vertex number
int graph_get_size (graph_t*);	// edge number

/***********************    Profiling    ****************************/
/**
 * Counts how many edges has a vertex.
 *
 * @param edge_dir Wether you want to count incoming edges, outcoming egdes, or all of them.
 */
int graph_get_vertex_grade_s(char* vertex_name, enum EdgeDirection edge_dir, graph_t*);
int graph_get_vertex_grade_i(int vertex_id, enum EdgeDirection edge_dir, graph_t*);
int graph_get_vertex_grade_v(vertex_t* v, enum EdgeDirection edge_dir, graph_t*);

/**
 * Computes the cluster coefficient of a vertex.
 * This value is the connectivity among its neighbors divided by the 
 * maximum possible connectivity among its neighbors
 * @return valid values are within [0,1], if return==-1 the vertex had 1 or less neighbors, making
 * the maximum possible edges == 0, triggering a division by 0
 */
float graph_get_vertex_clustering_coefficient_s(char* vertex_name, enum EdgeType edge_type, graph_t*);
float graph_get_vertex_clustering_coefficient_i(int vertex_id, enum EdgeType edge_type, graph_t*);
float graph_get_vertex_clustering_coefficient_v(vertex_t* v, enum EdgeType edge_type, graph_t*);

/**
 * Computes the average clustering coefficient of the graph.
 * This value is the summation of the clustering coefficient of all vertices
 * divided by the quantity of vertices.
 */
float graph_get_clustering_coefficient(enum EdgeType edge_type, graph_t*);

/**
 * Computes wether the graph can be divided in two disjoint subgraphs and every edge
 * connects both subgraphs. This means that each subgraph has a color, and any edge 
 * connects vertex of the same color.
 * @param color pointer to int array, shouldn't have memory allocated. It will have 
 * the color of the vertex i at color[i]. 0: not in graph, 1: color one, 2: color two.
 * @return 1: It is bipartite, 0: it is NOT bipartite.
 */
int graph_get_bipartiteness (int **color, enum EdgeType edge_type, graph_t* graph_p);

/**
 * @param inflation Granularity of the clustering. It is often called r. Good values are within [1.5, 5]
 * @param expansion Power parameter e. Usually 2 or 3. By the moment it's 2 hardcoded.
 * @return Graph with the same nodes, but linked in clusters.
 */
graph_t* graph_mcl(float inflation, int expansion, graph_t *graph_p);



void graph_plot(char* filename, enum Plot_Type, graph_t*);

graph_path_t * graph_run_dijkstra (vertex_t *orig, enum EdgeType edge_type, graph_t * graph_p);

void graph_run_path_stats(graph_t *graph_p);
void graph_run_grade_stats(graph_t *graph_p);

/**
 * Calculates the vertex disjoint subgraphs in the graph.
 * 
 * @param graph
 * @return list of lists of subgraphs
 */

array_list_t* graph_vertex_disjoint(graph_t *graph_p);
//void graph_run_spanning_tree(int vertex_id, int **subgraph, graph_t *graph_p);

#endif //_GRAPH_H_
