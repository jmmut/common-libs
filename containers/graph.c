#include "graph.h"


/**
 * Private declarations. Don't use out here
 * */
 typedef struct path_node{
    float distance;
    char visited;
    struct heap_node hn;
    int index;
} path_node_t;

int __graph_add_edge(int src, int dst, void* edge_data, enum EdgeType edge_type, float weight, graph_t* graph_p);
void __graph_print_dot(char* file_name, int print_weight, graph_t* graph_p);
int __dijkstra_compare_node(struct heap_node* n1,struct heap_node* n2);

/***********************    Creation and initialization    ****************************/
graph_t* graph_new(enum GraphType mask, int initial_num_vertices, int SYNC_MODE)
{
    graph_t *g = (graph_t*)calloc(1,sizeof(graph_t));
    
    if((mask & GRAPH_MIXED_DIRECTED) == GRAPH_MIXED_DIRECTED)
        g->directed = GRAPH_MIXED_DIRECTED;
    else if(mask & GRAPH_DIRECTED)
        g->directed = GRAPH_DIRECTED;
    else
        g->directed = GRAPH_NON_DIRECTED;   //default value
    
    g->cyclic = (mask & GRAPH_ACYCLIC)? 0: 1;
    //g->multiple = (mask & GRAPH_MULTIPLE)? 1: 0;
    g->multiple = 0;
    g->strict = mask & GRAPH_STRICT;
    g->non_negative = mask & GRAPH_NON_NEGATIVE_WEIGHT;
    
    g->num_edges = 0;
    g->num_vertices = 0;
    
    g->sync_mode = SYNC_MODE;
    g->vertices = array_list_new(initial_num_vertices,1.5,SYNC_MODE);
    g->removed_vertices = linked_list_new (SYNC_MODE);
    g->dict = kh_init(gr);
    
    g->stats.valid = 0;
    g->chunk = 50;
    return g;
    
}


graph_t *graph_create_free_scale (enum GraphType mask, int order, int SYNC_MODE, int m)
{
    graph_t* g = graph_new(mask, order, SYNC_MODE);
    enum EdgeType edge_type;
    int *appearances = (int*) malloc (sizeof(int)*order*2);
    int inserted;
    int i, j, k, end_pos = 0;
    char name[20];
    //srand(time(NULL));
    

    if (GRAPH_DIRECTED & mask)
        edge_type = GRAPH_EDGE_DIRECTED;
    else if (GRAPH_NON_DIRECTED & mask)
        edge_type = GRAPH_EDGE_NON_DIRECTED;

    
    for (i = 0; i < m+1; i++)
    {
        sprintf(name, "%d", i);
        graph_add_vertex(name, NULL, g);
    }
    
    for (i = 0; i < m+1; i++)
        for (j = i+1; j < m+1; j++)
        {
            graph_add_edge_iw (i, j, NULL, edge_type, 1, g);   // initial graph, full connectivity
            appearances[end_pos++] = i;
            appearances[end_pos++] = j;
        }

    for (i = m; i < order; i++)
    {
        sprintf(name, "%d", i);
        graph_add_vertex(name, NULL, g);
        inserted = rand()%end_pos;
        graph_add_edge_iw (i, appearances[inserted], NULL, edge_type, 1, g);   // initial graph, full connectivity
        appearances[end_pos++] = appearances[inserted];
        appearances[end_pos++] = i;
    }
    free(appearances);
    return g;
/*
    for (i = m; i < order; i++)
    {
        graph_add_vertex(itoa(i), NULL, g);
        for (j = 0; j < m; j++)
        {
            
            inserted[j] = rand()%end_pos;
            
            appearances[end_pos++] = i;
            appearances[end_pos++] = j;
            
    */
    
}
/***********************    Destruction    ****************************/

int graph_free(void (*vertex_data_callback) (void* vertex_data), void (*edge_data_callback) (void* edge_data), graph_t* graph_p)
{
    assert(graph_p);
    graph_clear(vertex_data_callback,edge_data_callback,graph_p);
    array_list_free(graph_p->vertices, NULL);
    linked_list_free(graph_p->removed_vertices, NULL);
    kh_destroy(gr, graph_p->dict);
    if(graph_p->stats.grade) free(graph_p->stats.grade);
    if(graph_p->stats.betweenness) free(graph_p->stats.betweenness);
    if(graph_p->stats.max_jumps) free(graph_p->stats.max_jumps);
    if(graph_p->stats.max_w) free(graph_p->stats.max_w);
    
    free(graph_p);
}


int graph_clear(void (*vertex_data_callback) (void* vertex_data), void (*edge_data_callback) (void* edge_data), graph_t* graph_p)
{
    assert(graph_p);
    int i;
    vertex_t *v;
    edge_t* e;
    linked_list_iterator_t *iter;
    
    if(graph_p->num_vertices)
        iter = (linked_list_iterator_t *)malloc(sizeof(linked_list_iterator_t));
    
    for (i = 0; i < graph_p->num_vertices; i++)
    {
        
        v = (vertex_t*)array_list_get(i, graph_p->vertices);
        if(v->src == NULL){
            free(v);
            continue;
        }
        if(edge_data_callback)
        {
            iter = linked_list_iterator_init(v->dst, iter); // free dst data
            e = (edge_t*) linked_list_iterator_curr(iter);
            
            while (e != NULL)
            {
                edge_data_callback(e->data);
                e = (edge_t*)linked_list_iterator_next(iter);
            }
            
            
            iter = linked_list_iterator_init(v->nd, iter);  // free nd data and edge (only when e->src_id==-1, else sets to -1)
            e = (edge_t*) linked_list_iterator_curr(iter);
            while (e != NULL)
            {
                if(e->src_id != -1)
                    e->src_id = -1;
                else{
                    edge_data_callback(e->data);
                    free(e);
                }
                e = (edge_t*)linked_list_iterator_next(iter);
                
            }
        }
        else
        {
            iter = linked_list_iterator_init(v->nd, iter); // free nd edge (only when e->src_id==-1, else sets to -1)
            e = (edge_t*) linked_list_iterator_curr(iter);
            while (e != NULL)
            {
                if(e->src_id != -1)
                    e->src_id = -1;
                else
                    free(e);
                
                e = (edge_t*)linked_list_iterator_next(iter);
            }
        }
        
        linked_list_free(v->src, NULL);
        linked_list_free(v->dst, free); // free dst edge
        linked_list_free(v->nd, NULL);
        
        free(v->name);
        if(vertex_data_callback)
            vertex_data_callback(v->data);
            
        free(v);
    
        
    }
    if(graph_p->num_vertices)
        linked_list_iterator_free(iter);
    array_list_clear(graph_p->vertices, NULL);
    linked_list_clear(graph_p->removed_vertices, NULL);
    
    kh_clear(gr, graph_p->dict);
    graph_p->num_edges = 0;
    graph_p->num_vertices = 0;

}

/***********************    Vertex Functions    ****************************/
 

int graph_find_vertex(char* name, graph_t* graph_p){
    assert(graph_p);
    
    khiter_t k = kh_get(gr,graph_p->dict,name);
    
    if(k == kh_end(graph_p->dict))  //If it was not found, ret -1
        return -1;
    else
        return kh_value(graph_p->dict, k);
}

inline vertex_t* graph_get_vertex_s(char* vertex_name, graph_t * graph_p)
{
    return graph_get_vertex_i (graph_find_vertex (vertex_name, graph_p), graph_p);
}
vertex_t* graph_get_vertex_i(int vertex_id, graph_t * graph_p)
{
    assert(graph_p);
    vertex_t* v = NULL;
    
    if (vertex_id < graph_p->num_vertices && vertex_id >= 0)
    {
        v = (vertex_t*)array_list_get(vertex_id, graph_p->vertices);
        if (v != NULL)
            if (v->src == NULL) // v is in the removed_vertex list
                v = NULL;
    }
    return v;
}

int graph_exists_vertex_s(char* name, graph_t* graph_p)
{
    int id = graph_find_vertex(name, graph_p);
    return graph_exists_vertex_i(id, graph_p);
}

int graph_exists_vertex_i(int id, graph_t* graph_p)
{
    if( graph_get_vertex_i(id, graph_p) != NULL)
        return 0;
    else
        return -1;
}


int graph_reachable_vertex(int src, int dst, graph_t* graph_p){
    
    if(src == dst)
        return 0;
    
    linked_list_t* queue = linked_list_new(graph_p->sync_mode);
    vertex_t* v, *v_dst;
    edge_t *e;
    linked_list_iterator_t *iter = (linked_list_iterator_t*)malloc(sizeof(linked_list_iterator_t));
    int i, fin = 0, dst_id;
    char *c = (char*)calloc(graph_p->num_vertices, sizeof(char));   // already visted or not yet

    v = graph_get_vertex_i(src, graph_p);
    v_dst = graph_get_vertex_i(dst, graph_p);
    if(v != NULL && v_dst != NULL)
    {
        linked_list_insert_last(v, queue);
        c[v->id] = 1;
        while(v = (vertex_t*)linked_list_remove_first(queue))
        {
            linked_list_iterator_init(v->dst, iter);
            e = (edge_t*)linked_list_iterator_curr(iter);
            while(e != NULL)
            {
                if(c[e->dst_id]==0){
                    if(e->dst_id == dst){
                        fin = 1;
                        break;
                    }
                    linked_list_insert_last(graph_get_vertex_i(e->dst_id, graph_p),queue);
                    c[e->dst_id] = 1;
                }
                e=(edge_t*)linked_list_iterator_next(iter);
            }
            if(!fin){
                linked_list_iterator_init(v->nd, iter);
                e = (edge_t*)linked_list_iterator_curr(iter);
                while(e != NULL)
                {
                    dst_id = (e->dst_id != v->id)? e->dst_id: e->src_id; 
                    if(c[dst_id]==0){
                        if(dst_id == dst){
                            fin = 1;
                            break;
                        }
                        linked_list_insert_last(graph_get_vertex_i(dst_id, graph_p),queue);
                        c[dst_id] = 1;
                    }
                    e=(edge_t*)linked_list_iterator_next(iter);
                }
            }
        }
    }
    linked_list_iterator_free(iter);
    linked_list_free(queue, NULL);
    free(c);
    return fin-1;
}



linked_list_t* graph_get_vertex_neighborhood_v(vertex_t* vertex_p, enum EdgeDirection edge_type, int k, graph_t* graph_p)
{
    assert(graph_p);
    if(vertex_p == NULL)
        return NULL;
    
    linked_list_t *queue = linked_list_new(COLLECTION_MODE_ASYNCHRONIZED); 
    if(k <= 0){
        linked_list_insert_last(vertex_p, queue);
        return queue;
    }
    
    khash_t(ii) *visited = kh_init(ii);
    
    linked_list_iterator_t *iter = linked_list_iterator_new(queue);
    linked_list_iterator_t *iter_edge = linked_list_iterator_new(queue);
    vertex_t *v = vertex_p;
    edge_t *e;
    
    int in = edge_type & GRAPH_EDGE_IN;
    int out = edge_type & GRAPH_EDGE_OUT;
    int dst, ret;
    
    //Insert a NULL to indicate the distance from the origin was increased.
    //You can know the distance to the origin counting the NULLs 
    //(in fact, you can't, because those NULL are deleted)
    linked_list_insert_last(v, queue);
    linked_list_insert_last(NULL, queue);
    linked_list_iterator_first(iter);
    
    kh_put(ii,visited,v->id,&ret);
    //printf("k = %d\n",k);
    while(1)
    {
        //printf("\n\n");
        if(v == NULL){
            k--;
            //printf("k = %d\n",k);
            linked_list_iterator_remove(iter);
            if(linked_list_iterator_curr(iter) == NULL) //If it was the last NULL, break;
                break;
        
            if(k <= 0)
                break;
            else
            {
                linked_list_insert_last(NULL, queue);
                v = (vertex_t*)linked_list_iterator_curr(iter);
                continue;
            }
        }
        
        //printf("--VERTEX-- %d\n", v->id);
        if(in)
        {
            linked_list_iterator_init(v->src, iter_edge);
            e = linked_list_iterator_curr(iter_edge);
            while(e != NULL)
            {
                kh_put(ii,visited,e->src_id,&ret);
                if(ret != 0)
                    linked_list_insert_last( array_list_get(e->src_id, graph_p->vertices), queue);  //inserts in the queue

                e= (edge_t*)linked_list_iterator_next(iter_edge);
            }
        }
        if(out)
        {
            linked_list_iterator_init(v->dst, iter_edge);
            e = linked_list_iterator_curr(iter_edge);
            while(e != NULL)
            {
                kh_put(ii,visited,e->dst_id,&ret);
                if(ret != 0)
                    linked_list_insert_last( array_list_get(e->dst_id, graph_p->vertices), queue);  //inserts in the queue
                
                e= (edge_t*)linked_list_iterator_next(iter_edge);
            }
        }
        if(in || out){
            linked_list_iterator_init(v->nd, iter_edge);
            e = linked_list_iterator_curr(iter_edge);
            while(e != NULL)
            {
                dst = (e->dst_id!=v->id)? e->dst_id: e->src_id;
                
                kh_put(ii,visited,dst,&ret);
                if(ret != 0){
                    linked_list_insert_last( array_list_get(dst, graph_p->vertices), queue);    //inserts in the queue
                }
                //printf("Inserted %d, ret=%d\n", dst, ret);

                e= (edge_t*)linked_list_iterator_next(iter_edge);
            }
        }
        
        v = (vertex_t*)linked_list_iterator_next(iter);
    }
    
    linked_list_iterator_free(iter);
    linked_list_iterator_free(iter_edge);
    kh_destroy(ii,visited);
    
    return queue;
}
inline linked_list_t* graph_get_vertex_neighborhood_s(char* name, enum EdgeDirection edge_type, int k_jumps, graph_t* graph_p)
{
    return graph_get_vertex_neighborhood_v(graph_get_vertex_s(name, graph_p), edge_type, k_jumps, graph_p);
}
inline linked_list_t* graph_get_vertex_neighborhood_i(int vertex_id, enum EdgeDirection edge_type, int k_jumps, graph_t* graph_p)
{
    return graph_get_vertex_neighborhood_v(graph_get_vertex_i(vertex_id, graph_p), edge_type, k_jumps, graph_p);
}

inline linked_list_t* graph_get_vertex_adjacents_v(vertex_t* vertex_p, graph_t* graph_p)
{
    return graph_get_vertex_neighborhood_v(vertex_p, GRAPH_EDGE_OUT, 1, graph_p);
}
inline linked_list_t* graph_get_vertex_adjacents_s(char* name, graph_t* graph_p)
{
    return graph_get_vertex_neighborhood_v(graph_get_vertex_s(name, graph_p), GRAPH_EDGE_OUT, 1, graph_p);
}
inline linked_list_t* graph_get_vertex_adjacents_i(int id, graph_t* graph_p)
{
    return graph_get_vertex_neighborhood_v(graph_get_vertex_i(id, graph_p), GRAPH_EDGE_OUT, 1, graph_p);
}

/*!
 * @abstract
 * 
 * @return  >= 0 : vertex index
 *          -1   : Error at array_list_insert
 *          -2   : Error at kh_put. It already exists on the hash_table
 *          -3   : Error name == NULL
 */
int graph_add_vertex(char* name_p, void* vertex_data, graph_t* graph_p){
    
    assert(graph_p);
    vertex_t *v;
    
    if (name_p == NULL)
        return -3;
    
    char *name = strndup (name_p, GRAPH_MAX_NAME_LENGTH +1);
    
    int ret;
    khiter_t k = kh_put(gr, graph_p->dict, name, &ret);
    if (!ret){
        //kh_del(gr, graph_p->dict, k);
        free(name);
        return -2;
    }
    
    if (linked_list_size(graph_p->removed_vertices))
    {
        v = linked_list_get_first(graph_p->removed_vertices);
        linked_list_remove_first(graph_p->removed_vertices);
    }
    else
    {
        v = (vertex_t*)malloc(sizeof(vertex_t));
        if(array_list_insert(v, graph_p->vertices) == 0){
            free(v);
            free(name);
            return -1;
        }
        
        graph_p->num_vertices++;
        v->id = array_list_size(graph_p->vertices)-1;
    }
    //int id = graph_find_vertex(name_p, g); // Even if it already exists, it has to create a new one
    
     

    //Created && appended OK

    
    v->data = vertex_data;
    
    v->src = linked_list_new(graph_p->sync_mode);
    v->dst = linked_list_new(graph_p->sync_mode);
    v->nd = linked_list_new(graph_p->sync_mode);
    
    v->name = name;
    
    kh_value(graph_p->dict,k) = v->id;
    
    return v->id;
}


/**
 * @return  0   OK
 *          -1  Not existing vertex
 */
inline int graph_remove_vertex_s(char* vertex_name, void (*vertex_data_callback) (void* vertex_data),void (*edge_data_callback) (void* edge_data), graph_t* graph_p)
{
    int vertex_id = graph_find_vertex(vertex_name,graph_p);
    return graph_remove_vertex_i(vertex_id, vertex_data_callback, edge_data_callback, graph_p);
}
int graph_remove_vertex_i(int vertex_id, void (*vertex_data_callback) (void* vertex_data),void (*edge_data_callback) (void* edge_data), graph_t* graph_p)
{
    assert(graph_p);
    if(vertex_id < 0 || vertex_id >= graph_p->num_vertices)
        return -1;
    
    vertex_t *v = graph_get_vertex_i(vertex_id, graph_p);
    if(v == NULL)
        return 0;
    linked_list_iterator_t *iter = linked_list_iterator_new(v->src);
    edge_t *e = (edge_t*)linked_list_iterator_curr(iter);
    while(e != NULL)
    {
        linked_list_iterator_next(iter);
        graph_remove_edge_e(e, GRAPH_EDGE_DIRECTED, edge_data_callback, graph_p);
        e = (edge_t*)linked_list_iterator_curr(iter);
    }
    
    iter = linked_list_iterator_init(v->dst, iter);
    e = linked_list_iterator_curr(iter);
    while(e != NULL)
    {
        linked_list_iterator_next(iter);
        graph_remove_edge_e(e, GRAPH_EDGE_DIRECTED, edge_data_callback, graph_p);
        e = (edge_t*)linked_list_iterator_curr(iter);
    }
    
    iter = linked_list_iterator_init(v->nd, iter);
    e = linked_list_iterator_curr(iter);
    while(e != NULL)
    {
        linked_list_iterator_next(iter);
        graph_remove_edge_e(e, GRAPH_EDGE_NON_DIRECTED, edge_data_callback, graph_p);
        e = (edge_t*)linked_list_iterator_curr(iter);
    }
    linked_list_iterator_free(iter);
    
    linked_list_free(v->src, NULL);
    linked_list_free(v->dst, NULL); // free dst edge
    linked_list_free(v->nd, NULL);
    v->src = v->dst = v->nd = NULL;
    
    kh_del(gr,graph_p->dict, kh_get(gr,graph_p->dict, v->name));
    
    free(v->name);
    v->name = NULL;
    if(vertex_data_callback)
        vertex_data_callback(v->data);
    v->data = NULL;
    linked_list_insert(v, graph_p->removed_vertices);
    
    return 0;
}

/***********************    Edge Functions    ****************************/



int __graph_add_edge(int src, int dst, void* edge_data, enum EdgeType edge_type, float weight, graph_t* graph_p){
    edge_t * e = (edge_t*)malloc(sizeof(edge_t));
    e->src_id = src;
    e->dst_id = dst;
    e->data = edge_data;
    e->weight = weight;
    
    switch(edge_type)
    {
        case GRAPH_EDGE_DIRECTED:
            if(graph_p->directed == GRAPH_NON_DIRECTED && graph_p->strict == GRAPH_STRICT){
                free(e);
                return -3;
            }

            linked_list_insert(e,(graph_get_vertex_i(src,graph_p))->dst);
            linked_list_insert(e,(graph_get_vertex_i(dst,graph_p))->src);
            break;
        case GRAPH_EDGE_NON_DIRECTED:
            if(graph_p->directed == GRAPH_DIRECTED && graph_p->strict == GRAPH_STRICT){
                free(e);
                return -3;
            }
            linked_list_insert(e,(graph_get_vertex_i(src,graph_p))->nd);
            if (src != dst) {
                linked_list_insert(e,(graph_get_vertex_i(dst,graph_p))->nd);
            }
            break;
        default:
            free(e);
            return -2;
    }
    graph_p->num_edges++;
    return 0;

}
void graph_aleale(int num_v, float connectivity, enum EdgeType edge_type, graph_t* g)
{
    int i = 0;
    int s;
    int zero = 0;
    int * ini = edge_type == GRAPH_EDGE_NON_DIRECTED? &s: &zero;
    for(s = 0; s < num_v; s++)
        for(int d = *ini; d < num_v; d++)
            if(rand()/(1.0*RAND_MAX) <= connectivity){
                __graph_add_edge(s,d,NULL,edge_type,/*5+rand()%40+*/1,g);
                i++;
                if(!(i % 1000000))
                    printf("Llevamos %iM edges puestos\n", i/1000000);
            }
    printf("%i edges puestos\n", i);
}
inline int graph_add_edge_i(int src, int dst, void* edge_data, enum EdgeType edge_type, graph_t* graph_p)
{
    return graph_add_edge_iw(src, dst, edge_data, edge_type, 1, graph_p);
}
inline int graph_add_edge_s(char* src, char* dst, void* edge_data, enum EdgeType edge_type, graph_t* graph_p)
{
    return graph_add_edge_sw(src, dst, edge_data, edge_type, 1, graph_p);
}
inline int graph_add_edge_sw(char* src, char* dst, void* edge_data, enum EdgeType edge_type, float weight, graph_t* graph_p)
{
    int s = graph_find_vertex(src,graph_p);
    int d = graph_find_vertex(dst,graph_p);
    return graph_add_edge_iw(s, d, edge_data, edge_type, weight, graph_p);
}
 
/**
 * 
 * 
 * @return 0    : OK
 *          -1   : src or dst are not in graph
 *          -2   : edge_type non supported
 *          -3   : edge_type non compatible with the graph directed type
 *          -4   : edge breaks acyclity
 *          -5   : edge breaks multiplicity
 *          -6   : Weight must be positive
 */
int graph_add_edge_iw(int src, int dst, void* edge_data, enum EdgeType edge_type, float weight, graph_t* graph_p){
    
    assert(graph_p);
    if (graph_get_vertex_i(src, graph_p) == NULL || graph_get_vertex_i(dst, graph_p) == NULL)
        return -1;
    
    
    
    //if(!graph_p->multiple)
        if(graph_get_edge_i(src,dst,edge_type, graph_p) != NULL)
            return -5;
    
    if(!graph_p->cyclic && graph_p->strict){//if acyclic
        if(edge_type == GRAPH_EDGE_NON_DIRECTED)
            if(graph_reachable_vertex(src, dst, graph_p)==0)
                return -4;
        if(graph_reachable_vertex(dst, src, graph_p)==0)
            return -4;
    }

    if(graph_p->non_negative && weight < 0) //FIXME weight < 0 || weight <= 0 ??
        return -6;

    //if(graph_exists_vertex_i(src, graph_p) < 0 || graph_exists_vertex_i(dst, graph_p) < 0 )
    //  return -6;
    return __graph_add_edge(src,dst,edge_data,edge_type,weight,graph_p);
    
}



inline edge_t* graph_get_edge_s(char* src, char* dst, enum EdgeType edge_type, graph_t* graph_p)
{
    vertex_t* v_src = graph_get_vertex_s (src, graph_p);
    vertex_t* v_dst = graph_get_vertex_s (dst, graph_p);
    return graph_get_edge_v (v_src, v_dst, edge_type, graph_p);
}

inline edge_t* graph_get_edge_i(int src, int dst, enum EdgeType edge_type, graph_t* graph_p)
{
    vertex_t* v_src = graph_get_vertex_i (src, graph_p);
    vertex_t* v_dst = graph_get_vertex_i (dst, graph_p);
    return graph_get_edge_v (v_src, v_dst, edge_type, graph_p);
}

edge_t* graph_get_edge_v(vertex_t* v_src, vertex_t* v_dst, enum EdgeType edge_type, graph_t* graph_p)
{
    assert(graph_p);
    linked_list_iterator_t *iter;
    
    if (v_src == NULL || v_dst == NULL)
        return NULL;
    
    edge_t *e;
    if(edge_type == GRAPH_EDGE_DIRECTED)
    {
        iter = linked_list_iterator_new(v_src->dst);
        e = (edge_t*)linked_list_iterator_curr(iter);
        while(e != NULL)
        {
            if(e->dst_id == v_dst->id)
            {
                linked_list_iterator_free(iter);
                return e;
            }
            e = (edge_t*)linked_list_iterator_next(iter);
        }
        linked_list_iterator_free(iter);
    }
    else if(edge_type == GRAPH_EDGE_NON_DIRECTED){
        int nd_dst;
        
        iter = linked_list_iterator_new(v_src->nd);
        e = (edge_t*)linked_list_iterator_curr(iter);
        while(e != NULL)
        {
            nd_dst = (e->src_id==v_src->id)? e->dst_id: e->src_id;
            if(nd_dst == v_dst->id)
            {
                linked_list_iterator_free(iter);
                return e;
            }
            e = (edge_t*)linked_list_iterator_next(iter);
        }
        linked_list_iterator_free(iter);
    }
    return NULL;
}


/*linked_list_t* graph_get_edge(int src, int dst, char edge_type, graph_t* graph_p)
{
    linked_list_t *l = linked_list_new(graph_p->sync_mode);
    if(array_list_size(graph_p->vertices) <= src || array_list_size(graph_p->vertices) <= dst || src < 0 || dst < 0)
        return l;
    
    linked_list_iterator_t *iter;
    
    vertex_t *v = (vertex_t*)graph_get_vertex_i(src,graph_p);
    edge_t *e;
    if(edge_type & GRAPH_DIRECTED)
    {
        iter = linked_list_iterator_new(v->dst);
        e = (edge_t*)linked_list_iterator_curr(iter);
        while(e != NULL)
        {
            if(e->dst_id == dst)
            {
                linked_list_insert(e->data,l);
            }
            e = (edge_t*)linked_list_iterator_next(iter);
        }
        linked_list_iterator_free(iter);
    }
    if(edge_type & GRAPH_NON_DIRECTED)
    {
        int nd_dst;
        
        iter = linked_list_iterator_new(v->nd);
        e = (edge_t*)linked_list_iterator_curr(iter);
        while(e != NULL)
        {
            nd_dst = (e->src_id==src)? e->dst_id: e->src_id;
            if(nd_dst == dst)
            {
                linked_list_insert(e->data,l);
            }
            e = (edge_t*)linked_list_iterator_next(iter);
        }
        linked_list_iterator_free(iter);
    }
    return l;
}
*/


/**
 * @return  0   OK
 *          -1  Edge doesn't exist
 *          -2  Corrupted edge
 */
inline int graph_remove_edge_s(char* src, char* dst, enum EdgeType edge_type, void (*edge_data_callback) (void* edge_data), graph_t* graph_p)
{
    edge_t* e = graph_get_edge_s( src, dst, edge_type, graph_p);
    return graph_remove_edge_e(e,edge_type, edge_data_callback, graph_p);
}
inline int graph_remove_edge_i(int src, int dst, enum EdgeType edge_type, void (*edge_data_callback) (void* edge_data), graph_t* graph_p)
{
    edge_t *e = graph_get_edge_i( src, dst, edge_type, graph_p);
    return graph_remove_edge_e(e,edge_type, edge_data_callback, graph_p);
}
int graph_remove_edge_e(edge_t *edge_p, enum EdgeType edge_type, void (*edge_data_callback) (void* edge_data), graph_t* graph_p)
{
    assert(graph_p);
    if(edge_p == NULL)
        return -1;
    
    
    linked_list_iterator_t *iter;
    vertex_t *v_src = graph_get_vertex_i(edge_p->src_id, graph_p);
    vertex_t *v_dst = graph_get_vertex_i(edge_p->dst_id, graph_p);
    if(v_src == NULL || v_dst == NULL)
        return -2;
    if(edge_type & GRAPH_EDGE_DIRECTED){
        linked_list_remove(edge_p, v_src->dst);
        linked_list_remove(edge_p, v_dst->src);
    }
    if(edge_type & GRAPH_EDGE_NON_DIRECTED)
    {
        linked_list_remove(edge_p, v_src->nd);
        linked_list_remove(edge_p, v_dst->nd);
    }
    if(edge_data_callback)
        edge_data_callback(edge_p->data);
    free(edge_p);
    graph_p->num_edges--;
    return 0;
}




/***********************    Others    ****************************/

void graph_print(graph_t* graph_p)
{
    assert(graph_p);
    int i;
    vertex_t *v;
    edge_t* e;
    linked_list_iterator_t *iter;
    if(graph_p->num_vertices)
        iter = (linked_list_iterator_t *)malloc(sizeof(linked_list_iterator_t));
    
    for (i = 0; i < graph_p->num_vertices; i++)
    {
        v = graph_get_vertex_i(i, graph_p);
        if(v == NULL)
            continue;
        printf("%s _____ id(%d)\n   src\n", v->name, v->id);
        
        iter = linked_list_iterator_init(v->src, iter);
        e = (edge_t*) linked_list_iterator_curr(iter);
        while (e != NULL)
        {
            printf ("\t%s <- %s\n"
                    , (graph_get_vertex_i(e->dst_id, graph_p))->name
                    , (graph_get_vertex_i(e->src_id, graph_p))->name);
            e = (edge_t*)linked_list_iterator_next(iter);
        }
        printf("   dst\n");
        
        iter = linked_list_iterator_init(v->dst, iter);
        e = (edge_t*) linked_list_iterator_curr(iter);
        while (e != NULL)
        {
            printf ("\t%s -> %s\n"
                    , (graph_get_vertex_i(e->src_id, graph_p))->name
                    , (graph_get_vertex_i(e->dst_id, graph_p))->name);
            e = (edge_t*)linked_list_iterator_next(iter);
        }
        printf("   nd\n");
        
        iter = linked_list_iterator_init(v->nd, iter);
        e = (edge_t*) linked_list_iterator_curr(iter);
        while (e != NULL)
        {
            printf ("\t%s -- %s\n"
                    , (graph_get_vertex_i(e->src_id, graph_p))->name
                    , (graph_get_vertex_i(e->dst_id, graph_p))->name);
            e = (edge_t*)linked_list_iterator_next(iter);
        }
        printf("\n");
        printf("\n");
    }
    
    if(graph_p->num_vertices)
        linked_list_iterator_free(iter);
}



void __graph_print_dot(char* file_name, int print_weight, graph_t* graph_p)
{
    assert(graph_p);
    FILE *f;
    int i;
    vertex_t *v;
    edge_t* e;
    linked_list_iterator_t *iter;
    
    if(file_name == NULL)
        f = stdout;
    
    else
        f = fopen(file_name, "w");
    
    if(graph_p->num_vertices)
        iter = (linked_list_iterator_t *)malloc(sizeof(linked_list_iterator_t));
    
    fprintf(f,"digraph {\n");

    for (i = 0; i < graph_p->num_vertices; i++)
    {
        v = graph_get_vertex_i(i, graph_p);
        if(v == NULL)
            continue;
        
        if(!linked_list_size(v->src) && !linked_list_size(v->dst) && !linked_list_size(v->nd))
            fprintf(f,"\t\"%s\";\n", v->name);
        
        iter = linked_list_iterator_init(v->dst, iter);
        e = (edge_t*) linked_list_iterator_curr(iter);
        while (e != NULL)
        {
            if(print_weight)
                fprintf(f,"\t\"%s\" -> \"%s\" [label=\"%.2f\"];\n"
                        , (graph_get_vertex_i(e->src_id, graph_p))->name
                        , (graph_get_vertex_i(e->dst_id, graph_p))->name
                        , e->weight);
            else
                fprintf(f,"\t\"%s\" -> \"%s\" ;\n"
                        , (graph_get_vertex_i(e->src_id, graph_p))->name
                        , (graph_get_vertex_i(e->dst_id, graph_p))->name);
                        
            e = (edge_t*)linked_list_iterator_next(iter);
        }
        
        iter = linked_list_iterator_init(v->nd, iter);
        e = (edge_t*) linked_list_iterator_curr(iter);
        while (e != NULL)
        {
            if( e->src_id == v->id)
            {
                if(print_weight)
                    fprintf(f,"\t\"%s\" -> \"%s\" [dir=none,label=\"%.2f\"];\n"
                        , (graph_get_vertex_i(e->src_id, graph_p))->name
                        , (graph_get_vertex_i(e->dst_id, graph_p))->name
                        , e->weight);
                else
                    fprintf(f,"\t\"%s\" -> \"%s\" [dir=none];\n"
                        , (graph_get_vertex_i(e->src_id, graph_p))->name
                        , (graph_get_vertex_i(e->dst_id, graph_p))->name);
            }
            e = (edge_t*)linked_list_iterator_next(iter);
        }
    }
        fprintf(f,"}\n");
    if(file_name != NULL)
        fclose(f);
    if(graph_p->num_vertices)
        linked_list_iterator_free(iter);
}
inline void graph_print_dot(char* file_name, graph_t* graph_p)
{
    __graph_print_dot(file_name, 0, graph_p);
}

inline void graph_print_dot_w(char* file_name, graph_t* graph_p)
{
    __graph_print_dot(file_name, 1, graph_p);
}


inline int graph_get_order (graph_t* graph_p)
{
    assert(graph_p);
    return graph_p->num_vertices - linked_list_size(graph_p->removed_vertices);
}
inline int graph_get_size (graph_t* graph_p)
{
    assert(graph_p);
    return graph_p->num_edges;
}

/***********************    Profiling    ****************************/

inline int graph_get_vertex_grade_s(char* vertex_name, enum EdgeDirection edge_dir, graph_t* graph_p)
{
    return graph_get_vertex_grade_v (graph_get_vertex_s (vertex_name, graph_p), edge_dir, graph_p);
}

inline int graph_get_vertex_grade_i(int vertex_id, enum EdgeDirection edge_dir, graph_t* graph_p)
{
    return graph_get_vertex_grade_v (graph_get_vertex_i (vertex_id, graph_p), edge_dir, graph_p);
}

int graph_get_vertex_grade_v(vertex_t * v, enum EdgeDirection edge_dir, graph_t* graph_p)
{
    assert(graph_p);
    if (v == NULL)
        return -1;
    
    int n_edges;

    n_edges = linked_list_size(v->nd);

    if (edge_dir & GRAPH_EDGE_IN)
        n_edges += linked_list_size(v->src);
    
    if (edge_dir & GRAPH_EDGE_OUT)
        n_edges += linked_list_size(v->dst);
    
    return n_edges;
}

inline float graph_get_vertex_clustering_coefficient_s (char* vertex_name, enum EdgeType edge_type, graph_t* graph_p)
{
    return graph_get_vertex_clustering_coefficient_v (graph_get_vertex_s (vertex_name, graph_p), edge_type, graph_p);
}

inline float graph_get_vertex_clustering_coefficient_i (int vertex_id, enum EdgeType edge_type, graph_t* graph_p)
{
    return graph_get_vertex_clustering_coefficient_v (graph_get_vertex_i (vertex_id, graph_p), edge_type, graph_p);
}

float graph_get_vertex_clustering_coefficient_v (vertex_t* v, enum EdgeType edge_type, graph_t* graph_p)
{
    assert(graph_p);
    if (v == NULL)
        return -1;

    int dir_mode = (edge_type & GRAPH_EDGE_DIRECTED) && (graph_p->directed & GRAPH_DIRECTED);
    int non_dir_mode = (edge_type & GRAPH_EDGE_NON_DIRECTED) && (graph_p->directed & GRAPH_NON_DIRECTED);

    linked_list_t * l = linked_list_new (graph_p->sync_mode);   // list of adjacent nodes

    linked_list_iterator_t * iter = linked_list_iterator_new(v->dst);
    edge_t *e;
    vertex_t *v_aux;
    int e_id;

    if (dir_mode && non_dir_mode)
    {
        linked_list_iterator_init (v->dst, iter);
        e = linked_list_iterator_curr (iter);
        while (e != NULL)
        {
            linked_list_insert (graph_get_vertex_i (e->dst_id, graph_p), l);
            e = linked_list_iterator_next(iter);
        }

        linked_list_iterator_init (v->nd, iter);
        e = linked_list_iterator_curr (iter);
        while (e != NULL)
        {
            e_id = (e->dst_id == v->id)? e->src_id: e->dst_id;
            v_aux = graph_get_vertex_i (e_id, graph_p);
            if (linked_list_contains (v_aux, l) == 0)   // insert unless it is already in the list
                linked_list_insert (v_aux, l);

            e = linked_list_iterator_next(iter);
        }
    }
    else if (dir_mode)
    {
        linked_list_iterator_init (v->dst, iter);
        e = linked_list_iterator_curr (iter);
        while (e != NULL)
        {
            linked_list_insert (graph_get_vertex_i (e->dst_id, graph_p), l);
            e = linked_list_iterator_next(iter);
        }
    }
    else if (non_dir_mode)
    {
        linked_list_iterator_init (v->nd, iter);
        e = linked_list_iterator_curr (iter);
        while (e != NULL)
        {
            e_id = (e->dst_id == v->id)? e->src_id: e->dst_id;
            linked_list_insert (graph_get_vertex_i (e_id, graph_p), l);
            e = linked_list_iterator_next(iter);
        }
    }

    int num_adjacent = linked_list_size (l);
    int max_edges = 0;
    int curr_edges = 0;

    if (num_adjacent <= 1)
    {
        linked_list_iterator_free(iter);
        linked_list_free (l, NULL);
        return -1;
    }

    if (dir_mode)
        max_edges += num_adjacent * (num_adjacent-1);
    if (non_dir_mode)
        max_edges += (num_adjacent * (num_adjacent-1))/2;

    linked_list_iterator_t* iter1 = linked_list_iterator_new (l);
    linked_list_iterator_t* iter2 = linked_list_iterator_new (l);
    vertex_t* v_neighbor1 = (vertex_t*)linked_list_iterator_curr(iter1);
    vertex_t* v_neighbor2;

    while(v_neighbor1 != NULL)  // travel upper triangular matrix
    {
        v_neighbor2 = (vertex_t*)linked_list_iterator_next(iter2);  // skips (v_neighbor1 == v_neighbor2) iteration
        while (v_neighbor2 != NULL)
        {
            if (non_dir_mode)
                if (graph_get_edge_v (v_neighbor1, v_neighbor2, GRAPH_EDGE_NON_DIRECTED, graph_p) != NULL)
                    curr_edges++;

            if (dir_mode)
            {
                if (graph_get_edge_v (v_neighbor1, v_neighbor2, GRAPH_EDGE_DIRECTED, graph_p) != NULL)
                    curr_edges++;
                if (graph_get_edge_v (v_neighbor2, v_neighbor1, GRAPH_EDGE_DIRECTED, graph_p) != NULL)
                    curr_edges++;
            }
            v_neighbor2 = (vertex_t*) linked_list_iterator_next (iter2);
        }

        v_neighbor2 = v_neighbor1 = (vertex_t*) linked_list_iterator_next (iter1);
    }
    linked_list_iterator_free(iter);
    linked_list_iterator_free(iter1);
    linked_list_iterator_free(iter2);
    linked_list_free (l, NULL);
    return (curr_edges/(float)max_edges);
}


float graph_get_clustering_coefficient (enum EdgeType edge_type, graph_t* graph_p)
{
    assert(graph_p);
    float cc = 0;
    float partial_cc;
    int num_adjacent, i;
    for (i = 0; i < graph_p->num_vertices; i++)
    {
        partial_cc = graph_get_vertex_clustering_coefficient_i(i, edge_type, graph_p);
        if (partial_cc >= 0)
            cc += partial_cc;
    }

    cc /= graph_get_order(graph_p);

    return cc;
}

int graph_get_bipartiteness (int **color_array, enum EdgeType edge_type, graph_t* graph_p)
{
    int *color = (int*) calloc (graph_p->num_vertices, sizeof(int));
    if (color_array != NULL)
        *color_array = color;
    linked_list_t *pending = linked_list_new(COLLECTION_MODE_ASYNCHRONIZED);    // colored items that have to color its neighbors
    vertex_t *v;
    edge_t *e;
    linked_list_iterator_t * iter = (linked_list_iterator_t *)malloc(sizeof(linked_list_iterator_t));
    int dir_mode = (edge_type & GRAPH_EDGE_DIRECTED) && (graph_p->directed & GRAPH_DIRECTED);
    int non_dir_mode = (edge_type & GRAPH_EDGE_NON_DIRECTED) && (graph_p->directed & GRAPH_NON_DIRECTED);
    int i;
    int color1 = 1, color2 = 2;
    int e_id;

    for (i = 0; i < graph_p->num_vertices; i++)
    {
        if (color[i] == 0)  // it is not colored yet, and neither are its neighbors, we are entering 
        {                   // a disconnected subgraph
            v = graph_get_vertex_i (i, graph_p);
            if (v != NULL)
            {
                linked_list_insert (v, pending);
                color[i] = color1;

                while (linked_list_size(pending) > 0)   // vertex in the pending list must be colored
                {
                    v = linked_list_remove_first (pending);

                    if (dir_mode)
                    {
                        iter = linked_list_iterator_init(v->dst, iter);
                        e = linked_list_iterator_curr (iter);
                        while (e != NULL)
                        {
                            if (color[e->dst_id])   // it is colored, it has passed through the pending list
                            {
                                if (color[e->dst_id] == color[v->id])   // odd cycle
                                {
                                    linked_list_free (pending, NULL);
                                    linked_list_iterator_free(iter);
                                    return 0;
                                }
                            }
                            else    // reached for the first time, uncolored
                            {
                                color[e->dst_id] = color[v->id] == color1? color2: color1;  
                                linked_list_insert (graph_get_vertex_i (e->dst_id, graph_p), pending);
                            }
                            e = linked_list_iterator_next(iter);
                        }

                        iter = linked_list_iterator_init(v->src, iter);
                        e = linked_list_iterator_curr (iter);
                        while (e != NULL)
                        {
                            if (color[e->src_id])
                            {
                                if (color[e->src_id] == color[v->id])   // odd cycle
                                {
                                    linked_list_free (pending, NULL);
                                    linked_list_iterator_free(iter);
                                    return 0;
                                }
                            }
                            else
                            {
                                color[e->src_id] = color[v->id] == color1? color2: color1;  
                                linked_list_insert (graph_get_vertex_i (e->src_id, graph_p), pending);
                            }
                            e = linked_list_iterator_next(iter);
                        }

                    }

                    if (non_dir_mode)
                    {
                        iter = linked_list_iterator_init(v->nd, iter);
                        e = linked_list_iterator_curr (iter);
                        while (e != NULL)
                        {
                            e_id = e->src_id == v->id? e->dst_id: e->src_id;
                            if (color[e_id])
                            {
                                if (color[e_id] == color[v->id])   // odd cycle
                                {
                                    linked_list_free (pending, NULL);
                                    linked_list_iterator_free(iter);
                                    return 0;
                                }
                            }
                            else
                            {
                                color[e_id] = color[v->id] == color1? color2: color1;  
                                linked_list_insert (graph_get_vertex_i (e_id, graph_p), pending);
                            }
                            e = linked_list_iterator_next(iter);
                        }

                    }
                }
            }
        }
    }
    linked_list_free (pending, NULL);
    linked_list_iterator_free(iter);
    return 1;   // all graph is colored bipartitedly
}


array_list_t ** __graph_mcl_get_edges(graph_t *graph_p) {
    array_list_t **edges = (array_list_t**) malloc (graph_p->num_vertices * sizeof(array_list_t*));  // array of array_list_t*
    vertex_t *v;
    edge_t *e, *e_copy;
    array_list_t *al;
    linked_list_iterator_t * iter =  (linked_list_iterator_t *)malloc(sizeof(linked_list_iterator_t));
    int i;
    int loop_present;   // if the vertex has not a loop we have to add one

    for (i = 0; i < graph_p->num_vertices; i++) {
        v = graph_get_vertex_i (i, graph_p);
        loop_present = 0;
        if (v != NULL) {
            al = array_list_new (linked_list_size(v->dst) + linked_list_size(v->nd), 1.5, COLLECTION_MODE_ASYNCHRONIZED);

            iter = linked_list_iterator_init(v->dst, iter);
            e = linked_list_iterator_curr (iter);
            while (e != NULL)
            {
                e_copy = (edge_t*) malloc (sizeof(edge_t));
                e_copy->src_id = e->src_id; // must be i
                e_copy->dst_id = e->dst_id;
                e_copy->weight = e->weight;
                array_list_insert (e_copy, al);
                if (e->src_id == e->dst_id)
                    loop_present = 1;
                e = linked_list_iterator_next(iter);
            }

            iter = linked_list_iterator_init(v->nd, iter);
            e = linked_list_iterator_curr (iter);
            while (e != NULL)
            {
                e_copy = (edge_t*) malloc (sizeof(edge_t));
                e_copy->src_id = i;
                e_copy->dst_id = e->dst_id == i? e->src_id: e->dst_id;
                e_copy->weight = e->weight;
                array_list_insert (e_copy, al);
                if (e->src_id == e->dst_id)
                    loop_present = 1;
                e = linked_list_iterator_next(iter);
            }

            if (loop_present == 0)
            {
                e_copy = (edge_t*) malloc (sizeof(edge_t));
                e_copy->src_id = i;
                e_copy->dst_id = i;
                e_copy->weight = 1;
                array_list_insert (e_copy, al);
            }

            edges[i] = al;
        } else {
            edges[i] = NULL;
        }
    }
    linked_list_iterator_free(iter);
    return edges;
}

/**
 * Computes the num_column-th column of the square of the sparse matrix.
 * Computes the num_column-th sparse matrix square column.
 *
 * (a x x)   (a x x)   (a*a+x*x ...
 * (- x x) * (- x x) = (x*b ...
 * (b x x)   (b x x)   (b*a+x*b ...
 * S(3,1) = A(3,1) * A(1,1) ...
 * S(e->dst_id, num_column) = A(e->dst_id, e->src_id == i) * A(e->src_id == i, num_column)
 * sol[e->dst_id] += e->weight * column[e->src_id]
 */
float * __graph_mcl_column_product(int expansion, int num_column, int length, array_list_t ** edges) {
    float *sol = (float*) malloc(length * sizeof(float));
    float *column = (float*) malloc(length * sizeof(float));
    array_list_t *al;
    edge_t *e;
    int i, j;
    for (i = 0; i < length; i++) {
        column[i] = 0;
        sol[i] = 0;
    }

    al = edges[num_column];
    for (j = 0; j < array_list_size (al); j++) {
        e = array_list_get (j, al);   // e = al->items[j]
        column[e->dst_id] += e->weight;
    }

    for (i = 0; i < length; i++) {  // travel all edges
        if (edges[i] != NULL) {
            al = edges[i];
            for (j = 0; j < array_list_size (al); j++) {
                e = array_list_get (j, al);   // e = al->items[j]
                sol[e->dst_id] += e->weight* column[e->src_id];  // note that: e->src_id == i
            }
        }
    }
    free(column);
    return sol;
}


array_list_t * __graph_mcl_inflation (float inflation, int num_column, int length, float *column) {
    array_list_t * al;
    float sum = 0, sum2 = 0;
    float avg;
    int i, staying = 0;
    float rescalation;
    edge_t * e;

    for (i = 0; i < length; i++) {  // inflation loop
        column[i] = pow(column[i], inflation);
        //column[i] = column[i]*column[i];
        sum += column[i];
    }

     //without purge
    al = array_list_new (5, 1.5, COLLECTION_MODE_ASYNCHRONIZED);
    if (sum != 0) {
        rescalation = 1.0 / sum;
        for (i = 0; i < length; i++) {  // rescalation loop
            e = (edge_t*) malloc (sizeof(edge_t));
            e->src_id = num_column;
            e->dst_id = i;
            e->weight = column[i] * rescalation;

            if (e->weight >= 0.01) {
                array_list_insert (e, al);
            } else {
                free (e);
            }
        }
    }
     // without purge
   /* 
    avg = sum / length;
    //avg = 1.0/length;
    if (sum != 0) {
        for (i = 0; i < length; i++) {  // rescalation loop
            if (column[i] >= avg) {
                column[i] /= sum;
                staying++;
                sum2 += column[i];
            } else {
                column[i] = -1;
            }
        }
    }
    
    rescalation = 1.0 / sum2;

    al = array_list_new (staying, 1.5, COLLECTION_MODE_ASYNCHRONIZED);
    
    for (i = 0; i < length; i++) { // purge loop
        if (column[i] > 0.01) {
            e = (edge_t*) malloc (sizeof(edge_t));
            e->src_id = num_column;
            e->dst_id = i;
            e->weight = column[i] * rescalation;
            array_list_insert (e, al);
        }

    }
    */
    return al;
}

int __graph_mcl_diff (array_list_t **edges, array_list_t **new_edges) {
    static int i = 0;
    if (++i == 5)
        return 0;
    return 1;
}

/*
 * TODO
 *      mcl diff
 *      array list bug
 *      paralelizacion (omp? sse?)
 */
graph_t* graph_mcl(float inflation, int expansion, graph_t *graph_p) {
    array_list_t **edges, **edges_tmp, **aux_ptr;   // array of "vertices" that are an array_list of edges
    float *column;
    int i=0, j, length, different;
    edge_t *e;
    graph_t *g2;
    

    length = graph_p->num_vertices;
    edges = __graph_mcl_get_edges (graph_p);    //initial load from the graph
    aux_ptr = (array_list_t**) malloc (length * sizeof(array_list_t*)); // aux_ptr reuses the last array
    do {
        edges_tmp = aux_ptr;
        
    //omp_set_num_threads(4);
    //printf ("%s:%d omp_get_num_threads() = %d\n", __FILE__, __LINE__, omp_get_num_threads());    // DEPURACION
//#pragma omp parallel for private(column, i) shared(edges_tmp) 
        for (i = 0; i < length; i++) {  // expansion, inflation and purge; segmentated in columns, instead of full matrix

            //printf ("%s:%d omp_get_thread_num = %d\n", __FILE__, __LINE__, omp_get_thread_num());  // DEPURACION
            column = __graph_mcl_column_product(expansion, i, length, edges);
            edges_tmp[i] = __graph_mcl_inflation (inflation, i, length, column); 
            free(column);
        }

        // edges_tmp is completed
        different = __graph_mcl_diff(edges, edges_tmp);
       
       /*
        g2 = graph_copy_vertices (graph_p);
    for (i = 0; i < length; i++) {
        for (j = 0; j < array_list_size(edges[i]); j++) {
            if (edges[i] != NULL) {
                e = array_list_get (j, edges[i]);
                __graph_add_edge (e->src_id, e->dst_id, NULL, GRAPH_EDGE_DIRECTED, e->weight, g2);
            }
        }
    }
    char nombre[20];
    static int asdf = 1;
    sprintf(nombre, "mcl-4iter%d.gv",asdf++);
    graph_print_dot_w (nombre, g2);
    // */

        
        for (i = 0; i < length; i++) {
            if (edges[i] != NULL) {
                array_list_free (edges[i], free);
            }
        }
        aux_ptr = edges;
        edges = edges_tmp;
    } while (different);

    g2 = graph_copy_vertices (graph_p);
    for (i = 0; i < length; i++) {
        for (j = 0; j < array_list_size(edges[i]); j++) {
            if (edges[i] != NULL) {
                e = array_list_get (j, edges[i]);
                __graph_add_edge (e->src_id, e->dst_id, NULL, GRAPH_EDGE_DIRECTED, e->weight, g2);
            }
        }
    }

    for (i = 0; i < length; i++) {
        if (edges[i] != NULL) {
            array_list_free (edges[i], free);
        }
    }
    free(edges);
    free(aux_ptr);
    return g2;
}


    //rellenar el array +
    //poner autobucles +
    //multiplicar matrices ^expansion +
    //inflar ^inflation+
    //rescalar +
    //purgar +
    //goto 1 +

graph_t *graph_copy_vertices (graph_t * graph_p) {
    graph_t *g2 = (graph_t*)calloc(1,sizeof(graph_t));
    int i, ret;
    khiter_t k;
    vertex_t *v_copy, *v;

    g2->directed = graph_p->directed;
    g2->cyclic = graph_p->cyclic;
    //g->multiple = (mask & GRAPH_MULTIPLE)? 1: 0;
    g2->multiple = graph_p->multiple;
    g2->strict = graph_p->strict;
    g2->non_negative = graph_p->non_negative;
    
    g2->num_edges = 0;
    g2->num_vertices = graph_p->num_vertices;
    
    g2->sync_mode = graph_p->sync_mode;
    g2->vertices = array_list_new(g2->num_vertices, 1.5, g2->sync_mode);
    g2->removed_vertices = linked_list_new (g2->sync_mode);
    g2->dict = kh_init(gr);
    
    g2->stats.valid = 0;
    g2->chunk = graph_p->chunk;

    for (i = 0; i < g2->num_vertices; i++) {
        v_copy = array_list_get (i, graph_p->vertices);
        if (v_copy->src == NULL) {   // removed vertex
            v->src = v->dst = v->nd = NULL;
            v->name = NULL;
            v->data = NULL;
            linked_list_insert(v, graph_p->removed_vertices);
        } else {
            v = (vertex_t*)malloc(sizeof(vertex_t));
            v->src = linked_list_new(graph_p->sync_mode);
            v->dst = linked_list_new(graph_p->sync_mode);
            v->nd = linked_list_new(graph_p->sync_mode);
            v->name = strdup(v_copy->name);
            v->data = NULL; // get duplicate pointer?
            k = kh_put(gr, g2->dict, v->name, &ret);
            kh_value(g2->dict,k) = v->id;
        }
        v->id = i;
        array_list_insert(v, g2->vertices);
    }
    return g2;

}



void graph_plot(char* filename, enum Plot_Type plot_type, graph_t* graph_p)
{
    
    FILE* f = fopen(filename, "w");
    
    fprintf(f, "#Script generated by graph.c\n");
    fprintf(f, "#To run: gnuplot %s --persist\n", filename);
    khash_t(ii) *histo = kh_init(ii);
    int* v = NULL;
    float* vf = NULL;
    
    float factor = 1;

    switch(plot_type)
    {
        case PLOT_GRADE:
            v = graph_p->stats.grade;
            fprintf(f,"set title 'Grade'\n");
            fprintf(f, "set xlabel 'Grade'\n");
            break;
        case PLOT_BETWEENNESS:
            v = graph_p->stats.betweenness;
            factor = 10000.0/(graph_p->num_vertices*graph_p->num_vertices);
            fprintf(f,"set title 'Betweenness'\n");
            fprintf(f, "set xlabel 'Betweenness'\n");
            break;
        case PLOT_JUMPS:
            fprintf(f,"set title 'Jumps'\n");
            fprintf(f, "set xlabel 'Jumps'\n");
            v = graph_p->stats.max_jumps;
            break;
        case PLOT_NON_REACHABLE:
            v = graph_p->stats.non_reach;
            //factor = 1000.0/(graph_p->num_vertices);
            fprintf(f,"set title 'Non Reachable'\n");
            fprintf(f, "set xlabel 'Non Reachable'\n");
            break;
        case PLOT_WEIGHT:
            fprintf(f,"set title 'Weight'\n");
            fprintf(f, "set xlabel 'Weight'\n");
            vf = graph_p->stats.max_w;
            break;
        default:
            break;
        
    }
    khiter_t k;
    int ret;
    
    if(v)
        for(int i = 0; i < graph_p->num_vertices; i++)
        {
            if(!graph_p->stats.non_isolated[i])
                continue;

            k = kh_get(ii,histo,(int)v[i]*factor);
            if(kh_end(histo) == k){
                k = kh_put(ii, histo, (int)v[i]*factor ,&ret);
                kh_val(histo, k) = 0;
            }
            kh_val(histo, k)++;
        }
    else
        for(int i = 0; i < graph_p->num_vertices; i++)
        {
            if(!graph_p->stats.non_isolated[i])
                continue;
            
            k = kh_get(ii,histo,(int)(vf[i]*factor));
            if(kh_end(histo) == k){
                k = kh_put(ii, histo, (int)(vf[i]*factor) ,&ret);
                kh_val(histo, k) = 0;
            }
            kh_val(histo, k)++;
        }

    fprintf(f, "set ylabel 'Number of nodes'\n");
    fprintf(f, "set boxwidth 1\n");
    fprintf(f, "set style fill solid border -1\n");
    fprintf(f, "plot '-' using 1:2 with boxes\n");
    for(k = kh_begin(histo); k != kh_end(histo); k++){
        if (!kh_exist(histo,k)) continue;
        fprintf(f, "%d %d\n", kh_key(histo,k), kh_val(histo,k));
    }
    fprintf(f, "e\n");
    
    fclose(f);
    kh_destroy(ii, histo);
}

int __dijkstra_compare_node(struct heap_node* n1,struct heap_node* n2)
{
    return ((path_node_t*)(n1->value))->distance < ((path_node_t*)(n2->value))->distance;
}

/*
graph_path_t * graph_run_dijkstra (vertex_t *orig, enum EdgeType edge_type, graph_t * graph_p)
{
    assert(graph_p->non_negative);

    graph_path_t * path_l = (graph_path_t*)malloc(sizeof(graph_path_t)*graph_p->num_vertices);
    path_node_t * dists = (path_node_t*) calloc (graph_p->num_vertices, sizeof(path_node_t));   // distances table
    path_node_t * pnode;
    linked_list_iterator_t *iter_edge = (linked_list_iterator_t*)malloc(sizeof(linked_list_iterator_t));   // edge iterator within a vertex
    struct heap *h = malloc(sizeof(struct heap));
    struct heap_node *hn;
    int directed_path = edge_type & GRAPH_EDGE_DIRECTED;
    int non_directed_path = edge_type & GRAPH_EDGE_NON_DIRECTED;
    vertex_t *v;
    edge_t * e;
    
    float dist_aux;
    int i, e_id;
    for (i = 0; i < graph_p->num_vertices; i++)
    {
        path_l[i].distance = dists[i].distance = FLT_MAX;
        path_l[i].father = -1;
        dists[i].visited = 0;
        dists[i].index = i;
        
    }

    dists[orig->id].distance = 0;
    path_l[orig->id].father = orig->id;
    dists[orig->id].visited = 1;
    
    heap_init(h);

    v = orig;
    int CUENTA = 0;
    while (1)
    {
        dists[v->id].visited = 2;
        if(directed_path)
        {
            linked_list_iterator_init(v->dst, iter_edge);
            e = linked_list_iterator_curr(iter_edge);
            
            while(e != NULL)
            {CUENTA++;
                e_id = e->dst_id;
                pnode = &dists[e_id];
                dist_aux = dists[v->id].distance + e->weight;
                if (pnode->visited == 0)
                {
                    heap_node_init(&(pnode->hn), pnode);
                    
                    path_l[e_id].father = v->id;
                    pnode->distance = dist_aux;
                    
                    pnode->visited = 1;
                    heap_insert(__dijkstra_compare_node,h,&(pnode->hn));
                    
                }else if (pnode->distance > dist_aux)   // better path found
                {
                    pnode->distance = dist_aux;
                    path_l[e_id].father = v->id;
                    heap_decrease(__dijkstra_compare_node, h,&pnode->hn); 
                }
                e = (edge_t*)linked_list_iterator_next(iter_edge);
            }
        }
        //printf("\n");
        
        //printf("ID\t\tDist\t\tFather\t visited --------- v->name %s\n", v->name);
        //for (i = 0; i < graph_p->num_vertices; i++)
        //{
            //printf("%s\t\t%.0f\t\t%d\t%d\n", ((vertex_t*) (graph_p->vertices->items[i]))->name , dists[i].distance, path_l[i].father, dists[i].visited);
        //}printf("\n\n");
        
        
        if(non_directed_path)
        {CUENTA++;
            linked_list_iterator_init(v->nd, iter_edge);
            e = linked_list_iterator_curr(iter_edge);
            
            while(e != NULL)
            {
                e_id = (e->dst_id == v->id)? e->src_id: e->dst_id;
                pnode = &dists[e_id];
                dist_aux = dists[v->id].distance + e->weight;
                if (pnode->visited == 0)
                {
                    heap_node_init(&(pnode->hn), pnode);
                    
                    path_l[e_id].father = v->id;
                    pnode->distance = dist_aux;
                    
                    pnode->visited = 1;
                    heap_insert(__dijkstra_compare_node,h,&(pnode->hn));
                    
                }else if (pnode->distance > dist_aux)   // better path found
                {
                    pnode->distance = dist_aux;
                    path_l[e_id].father = v->id;
                    heap_decrease(__dijkstra_compare_node, h,&pnode->hn); 
                }
                e = (edge_t*)linked_list_iterator_next(iter_edge);
            }
        }
         
        path_l[v->id].distance = dists[v->id].distance;
        hn = heap_take(__dijkstra_compare_node, h);
        
        if(hn)
            v = array_list_get(((path_node_t*)(hn->value))->index, graph_p->vertices);
        else
            break;

    }
    //printf("CUENTA = %d \n" , CUENTA);
    free(dists);
    free(h);
    linked_list_iterator_free(iter_edge);
    return path_l;
}
/**/
/**/
graph_path_t * graph_run_dijkstra (vertex_t *orig, enum EdgeType edge_type, graph_t * graph_p)
{
    assert(graph_p->non_negative);

    graph_path_t * path_l = (graph_path_t*)malloc(sizeof(graph_path_t)*graph_p->num_vertices);
    path_node_t * dists = (path_node_t*) calloc (graph_p->num_vertices, sizeof(path_node_t));   // distances table
    path_node_t * pnode;
    //linked_list_iterator_t *iter_edge = (linked_list_iterator_t*)malloc(sizeof(linked_list_iterator_t));   // edge iterator within a vertex
    linked_list_item_t *l_item;
    struct heap *h = malloc(sizeof(struct heap));
    struct heap_node *hn;
    int directed_path = edge_type & GRAPH_EDGE_DIRECTED;
    int non_directed_path = edge_type & GRAPH_EDGE_NON_DIRECTED;
    vertex_t *v;
    edge_t * e;
    
    float dist_aux;
    int i, e_id;
    for (i = 0; i < graph_p->num_vertices; i++)
    {
        path_l[i].distance = dists[i].distance = FLT_MAX;
        path_l[i].father = -1;
        dists[i].visited = 0;
        dists[i].index = i;
    }

    dists[orig->id].distance = 0;
    path_l[orig->id].father = orig->id;
    dists[orig->id].visited = 1;
    
    heap_init(h);

    v = orig;
    int CUENTA = 0;
    while (1)
    {
        dists[v->id].visited = 2;
        if(directed_path)
        {
            
            //linked_list_iterator_init(v->dst, iter_edge);
            //e = linked_list_iterator_curr(iter_edge);
            //e = linked_list_get_first(v->dst);
            l_item = v->dst->first;
            //while(e != NULL)
            for(int iter = 0; iter < v->dst->size; iter++)
            {CUENTA++;
                e = l_item->item;
                e_id = e->dst_id;
                pnode = &dists[e_id];
                dist_aux = dists[v->id].distance + e->weight;
                if (pnode->visited == 0)
                {
                    heap_node_init(&(pnode->hn), pnode);
                    
                    path_l[e_id].father = v->id;
                    pnode->distance = dist_aux;
                    
                    pnode->visited = 1;
                    heap_insert(__dijkstra_compare_node,h,&(pnode->hn));
                    
                }else if (pnode->distance > dist_aux)   // better path found
                {
                    pnode->distance = dist_aux;
                    path_l[e_id].father = v->id;
                    heap_decrease(__dijkstra_compare_node, h,&pnode->hn); 
                }
                l_item = l_item->next;
                //e = (edge_t*)linked_list_iterator_next(iter_edge);
            }
        }
        //printf("\n");
        
        //printf("ID\t\tDist\t\tFather\t visited --------- v->name %s\n", v->name);
        //for (i = 0; i < graph_p->num_vertices; i++)
        //{
            //printf("%s\t\t%.0f\t\t%d\t%d\n", ((vertex_t*) (graph_p->vertices->items[i]))->name , dists[i].distance, path_l[i].father, dists[i].visited);
        //}printf("\n\n");
        
        
        if(non_directed_path)
        {CUENTA++;
            //linked_list_iterator_init(v->nd, iter_edge);
            //e = linked_list_iterator_curr(iter_edge);
            
            l_item = v->nd->first;
            for(int iter = 0; iter < v->nd->size; iter++)
            {
                e = l_item->item;                
                e_id = (e->dst_id == v->id)? e->src_id: e->dst_id;
                pnode = &dists[e_id];
                dist_aux = dists[v->id].distance + e->weight;
                if (pnode->visited == 0)
                {
                    heap_node_init(&(pnode->hn), pnode);
                    
                    path_l[e_id].father = v->id;
                    pnode->distance = dist_aux;
                    
                    pnode->visited = 1;
                    heap_insert(__dijkstra_compare_node,h,&(pnode->hn));
                    
                }else if (pnode->distance > dist_aux)   // better path found
                {
                    pnode->distance = dist_aux;
                    path_l[e_id].father = v->id;
                    heap_decrease(__dijkstra_compare_node, h,&pnode->hn); 
                }
                //e = (edge_t*)linked_list_iterator_next(iter_edge);
                l_item = l_item->next;
            }
        }
         
        path_l[v->id].distance = dists[v->id].distance;
        hn = heap_take(__dijkstra_compare_node, h);
        
        if(hn)
            v = array_list_get(((path_node_t*)(hn->value))->index, graph_p->vertices);
        else
            break;

    }
    //printf("CUENTA = %d \n" , CUENTA);
    free(dists);
    free(h);
    //linked_list_iterator_free(iter_edge);
    return path_l;
}/**/

void graph_run_grade_stats(graph_t *graph_p)
{
    if(!graph_p->stats.grade) graph_p->stats.grade = calloc(graph_p->num_vertices,sizeof(int));
    for(int i = 0; i < graph_p->num_vertices; i++)
        graph_p->stats.grade[i] = graph_get_vertex_grade_i(i, GRAPH_EDGE_OUT, graph_p);
    
}
    //khash_t(ii) disjoint = kh_init(ii);
    /* TODO
     * 
     * Init: List with all vertex
     * For every vertex: If the vertex is in the list, remove all rechables vertices from the list
     * Result: Number of elements in list == number of disjoint subgraphs
     * 
     * //if(me hayo en la lista)
     * //{
     *     //actualizar lista, segun mi path
     *     //{
     *         //recorrer lista. Eliminar si path[iteradot].dist != INF
     *     //}
     * //}
     * 
     * */

void graph_run_path_stats(graph_t *graph_p)
{
    graph_path_t *path;
    vertex_t *v;
    int v_it, i, ret, is_disjoint;

    //khash_t(ii) *disjoint = kh_init(ii);
    //int is_disjoint, k;
    //path = graph_run_dijkstra(array_list_get(0, graph_p->vertices), GRAPH_EDGE_ALL, graph_p);
    //for(i = 1; i < graph_p->num_vertices; i++)
    //{
        //if(path[i].distance == FLT_MAX)
            //kh_put(ii,disjoint,i, &ret);
    //}free(path);
    
    int max_jumps_path;
    int jumps_path;
    float max_w_path;
    
    if(graph_p->stats.grade) free(graph_p->stats.grade);
    if(graph_p->stats.betweenness) free(graph_p->stats.betweenness);
    if(graph_p->stats.max_jumps) free(graph_p->stats.max_jumps);
    if(graph_p->stats.max_w) free(graph_p->stats.max_w);
    if(graph_p->stats.non_reach) free(graph_p->stats.non_reach);
    if(graph_p->stats.non_isolated) free(graph_p->stats.non_isolated);

    graph_stats_t *stats = NULL;
    
    int   *grade        = calloc(graph_p->num_vertices,sizeof(int));
    int   *betweenness  = calloc(graph_p->num_vertices,sizeof(int));
    int   *max_jumps    = calloc(graph_p->num_vertices,sizeof(int));
    float *max_w        = calloc(graph_p->num_vertices,sizeof(int));
    int *non_reach    = calloc(graph_p->num_vertices,sizeof(int));
    int *non_isolated    = calloc(graph_p->num_vertices,sizeof(int));
    

    //printf("Number of threads: %d\n", omp_get_num_threads());

    linked_list_t *list = linked_list_new(COLLECTION_MODE_SYNCHRONIZED);
    
#pragma omp parallel for shared(graph_p, list, grade, max_jumps, max_w/*, disjoint*/)\
            private(i, v, path, max_jumps_path, max_w_path, jumps_path, v_it, /*is_disjoint, k*/) \
            firstprivate(stats) \
            schedule(static, graph_p->chunk)
    for(i = 0; i < graph_p->num_vertices; i++)
    {
        //if(kh_get(ii,disjoint,i) != kh_end(disjoint))
            //is_disjoint = 1;
        //else
            //is_disjoint = 0;
        
        
        max_jumps_path = 0;
        max_w_path = 0;
        if(stats == NULL)
        {
            stats = calloc(1, sizeof(graph_stats_t));
            //printf("TID: %d, i:%d \n", omp_get_thread_num(), i);
            //stats->grade  = calloc(graph_p->num_vertices,sizeof(int));
            stats->betweenness  = calloc(graph_p->num_vertices,sizeof(int));
            //stats->max_jumps  = calloc(graph_p->num_vertices,sizeof(int));
            //stats->max_w  = calloc(graph_p->num_vertices,sizeof(int));
            linked_list_insert(stats, list);
        }

        //if(!(i%200))
            //printf("Dijkstra %d \n",i);
        
        v = graph_get_vertex_i(i, graph_p);
        if(!v)
            continue;
        path = graph_run_dijkstra(v, GRAPH_EDGE_ALL, graph_p);
        
        for(int j = v_it = 0; j < graph_p->num_vertices; v_it = ++j){
            if(v_it == v->id) continue;
            
            
            
            if(path[v_it].distance != FLT_MAX){
                //if(is_disjoint)
                //{
                    //if((k = kh_get(ii,disjoint,i)) != kh_end(disjoint))
                        //kh_del(ii, disjoint, k);
                //}
                non_isolated[v->id] = 1;
                jumps_path = 0;
                while(v_it != v->id){   //follow the path
                    stats->betweenness[v_it]++;
                    jumps_path++;
                    v_it = path[v_it].father;
                }
                //stats->betweenness[v->id]++;
                
                if(path[j].distance > max_w_path)
                    max_w_path = path[j].distance;

                if(jumps_path > max_jumps_path)
                    max_jumps_path = jumps_path;
                
            }
            else
                non_reach[v->id]++;
            
            if(max_w_path > stats->max_w_path)
                stats->max_w_path = max_w_path;
            if(max_jumps_path > stats->max_jumps_path)
                stats->max_jumps_path = max_jumps_path;
        }
        
        /*stats->*/max_jumps[v->id] = max_jumps_path;
        /*stats->*/max_w[v->id] = max_w_path;
        //stats->max_jumps[v->id] = max_jumps_path;
        //stats->max_w[v->id] = max_w_path;
        
        free(path);
        
        //stats->grade[i] = graph_get_vertex_grade_i(i, GRAPH_EDGE_OUT, graph_p);
        /*stats->*/grade[i] = graph_get_vertex_grade_i(i, GRAPH_EDGE_OUT, graph_p);
        
    }

    int   max_jumps_total = 0;
    float max_w_total = 0;
    float max_bt = 0;
    int size = list->size;
    
    for(int j = 0; j < size; j++)
    {
        stats = linked_list_remove_first(list);
        for(i = 0; i < graph_p->num_vertices; i++){
            betweenness[i] += stats->betweenness[i];
        }
        if(stats->max_jumps_path > max_jumps_total)
            max_jumps_total = stats->max_jumps_path;
        if(stats->max_w_path > max_w_total)
            max_w_total = stats->max_w_path;
    }

    //for(int i = 0; i < graph_p->num_vertices; i++)
        //if(betweenness[i] > max_bt)
                //max_bt = betweenness[i];
    ////printf("MAXIMO BT: %d\n", max_bt);
    //max_bt /=100.0;
    ////printf("MAXIMO BT: %d\n", max_bt);


    //float p = graph_p->num_vertices*(graph_p->num_vertices/10000.0);
    //for(int i = 0; i < graph_p->num_vertices; i++)
        //betweenness[i] /= p;

    graph_p->stats.valid = 1;
    graph_p->stats.grade = grade; 
    graph_p->stats.betweenness = betweenness;
    graph_p->stats.max_jumps = max_jumps ;
    graph_p->stats.max_w     = max_w     ;
    graph_p->stats.max_jumps_path = max_jumps_total;
    graph_p->stats.max_w_path     = max_w_total;
    graph_p->stats.non_reach     = non_reach;
    graph_p->stats.non_isolated     = non_isolated;
    
  //  kh_destroy(ii, disjoint);
    
}


array_list_t* graph_vertex_disjoint(graph_t *graph_p)
{
    array_list_t *disjoint = array_list_new(1,1.5,graph_p->sync_mode);
    linked_list_t *l;
    subgraph_t *subg;
    linked_list_iterator_t *iter = malloc(sizeof(linked_list_iterator_t));
    int *spt = calloc(graph_p->num_vertices, sizeof(int));
    vertex_t *v;
    int v_id = 0;
    
    while(v_id < graph_p->num_vertices)
    {
        if(!spt[v_id]){        
            subg = malloc(sizeof(subgraph_t));
            l = graph_get_vertex_neighborhood_i(v_id,GRAPH_EDGE_ALL,graph_p->num_vertices, graph_p);
            array_list_insert(l, disjoint);
            
            subg->vertices = l;
            subg->num_vertices = l->size;
            
            linked_list_iterator_init(l,iter);
            v = linked_list_iterator_curr(iter);
            while(v)
            {
                spt[v->id]=1;
                v = linked_list_iterator_next(iter);
            }
        }
        v_id++;
    }
    free(spt);
    return disjoint;
}
/*
void graph_run_spanning_tree(int vertex_id, int **subgraph, graph_t *graph_p)
{
    int *spt = calloc(graph_p->num_vertices, sizeof(int));
    linked_list_t* queue = linked_list_new(COLLECTION_MODE_ASYNCHRONIZED);
    vertex_t *v = array_list_get(vertex_id, graph_p->vertices);
    spt[vertex_id] = 1;
    
    int i;
    linked_list_iterator_t *iter = malloc(sizeof(linked_list_iterator_t));
    while(v)
    {
        linked_list_iterator_init(v->dst, iter);
        e = (edge_t*)linked_list_iterator_curr(iter);
        while(e != NULL)
        {
            if(c[e->dst_id]==0){
                if(e->dst_id == dst){
                    fin = 1;
                    break;
                }
                linked_list_insert_last(graph_get_vertex_i(e->dst_id, graph_p),queue);
                c[e->dst_id] = 1;
            }
            e=(edge_t*)linked_list_iterator_next(iter);
        }
    }
}*/

