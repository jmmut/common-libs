
#include <stdio.h>
#include <string.h>
#include <check.h>

#include "../graph.h"
graph_t *g;
Suite *create_test_suite(void);

//**************************
//     Checked fixtures    *
//**************************

#define ged(s,d) graph_add_edge_i(s,d,NULL,GRAPH_EDGE_DIRECTED,g)
#define ge(s,d) graph_add_edge_i(s,d,NULL,GRAPH_EDGE_NON_DIRECTED,g)

#define geds(s,d,w) graph_add_edge_sw(#s,#d,NULL,GRAPH_EDGE_DIRECTED,w,g)
#define ges(s,d,w) graph_add_edge_sw(#s,#d,NULL,GRAPH_EDGE_NON_DIRECTED,w,g)


#define LINKED_LIST_FOR_EACH(void_p,list, code) {               \
            linked_list_item_t *__it__ = list->first;         \
            while(__it__ != NULL){                             \
                void_p = __it__->item;                          \
                __it__ = __it__->next;                          \
                code;                                           \
            }                                                   \
        }
#define ARRAY_LIST_FOR_EACH(void_p, array, code)                \
            for(int __i__ = 0; __i__ < array->size; __i__++){  \
                void_p = array->items[__i__];                   \
                code;                                           \
            }
/*
 *  vertex_t *v;
 *  edge_t*e;
 *  LINKED_LIST_FOR_EACH(v, g->vertices, 
 *  {
 *      printf("Vertice n = %s\n", v->name);
 *      LINKED_LIST_FOR_EACH(e, v->nd, 
 *      {
 *          printf("");
 *          
 *      });
 *  });
 */
void create_graph()
{
    g = graph_new(GRAPH_MIXED_DIRECTED | GRAPH_CYCLIC| GRAPH_NON_NEGATIVE_WEIGHT, 20, COLLECTION_MODE_ASYNCHRONIZED);

    graph_add_vertex("A", NULL, g);
    graph_add_vertex("B", NULL, g);
    graph_add_vertex("C", NULL, g);
    graph_add_vertex("D", NULL, g);
    graph_add_vertex("E", NULL, g);
    graph_add_vertex("F", NULL, g);
    graph_add_vertex("g", NULL, g);
    graph_add_vertex("h", NULL, g);

    geds(A,C,1);
    geds(A,B,50);
    geds(B,D,1);
    geds(C,B,1);
    geds(C,F,30);
    geds(F,E,1);
    geds(D,E,1);
    geds(g,h,1);
    geds(h,g,1);
}

void free_graph()
{
    graph_free(NULL, NULL, g);
}


START_TEST(test_empty_graph) {

    g = graph_new(GRAPH_DIRECTED | GRAPH_CYCLIC, 20, COLLECTION_MODE_ASYNCHRONIZED);

    fail_if( graph_remove_vertex_i(0, NULL, NULL, g) != -1, "Removing inexistent vertex: return should be -1");
    fail_if( graph_remove_edge_i(0, 1, GRAPH_EDGE_DIRECTED, NULL, g) != -1, "Removing inexistent vertex: return should be -1");
    fail_if( graph_remove_edge_i(0, 0, GRAPH_EDGE_DIRECTED, NULL, g) != -1, "Removing inexistent vertex: return should be -1");

    graph_free(NULL, NULL, g);
}
END_TEST


START_TEST(test_small_graph) {

    g = graph_new(GRAPH_DIRECTED | GRAPH_ACYCLIC |GRAPH_STRICT, 20, COLLECTION_MODE_ASYNCHRONIZED);
    int ret = graph_add_vertex("A", NULL, g);

    fail_if( ret < 0, "add_vertex: return should be the new id but id=%d", ret);
    ret = graph_add_vertex("B", NULL, g);
    fail_if( ret < 0, "add_vertex: return should be the new id but id=%d", ret);
    ret = graph_add_vertex("C", NULL, g);
    fail_if( ret < 0, "add_vertex: return should be the new id but id=%d", ret);


    ret = graph_add_edge_sw("A", "B", NULL, GRAPH_EDGE_DIRECTED, 2, g);
    fail_if( ret != 0, "add_vertex: return should be 0 but ret=%d", ret);

    ret = graph_add_edge_sw("D", "B", NULL, GRAPH_EDGE_DIRECTED, 2, g);
    fail_if( ret != -1, "add_vertex: non existent vertex: return should be -1 but ret=%d", ret);

    ret = graph_add_edge_sw("A", "C", NULL, 9, 2, g);
    fail_if( ret != -2, "add_vertex: non supported edge type: return should be -2 but ret=%d", ret);

    ret = graph_add_edge_sw("C", "A", NULL, GRAPH_EDGE_NON_DIRECTED, 2, g);
    fail_if( ret != -3, "add_vertex: non compatible direction type: return should be -3 but ret=%d", ret);

    ret = graph_add_edge_sw("B", "B", NULL, GRAPH_EDGE_DIRECTED, 2, g);
    fail_if( ret != -4, "add_vertex: breaking acyclity: return should be -4 but ret=%d", ret);

    ret = graph_add_edge_sw("A", "B", NULL, GRAPH_EDGE_DIRECTED, 2, g);
    fail_if( ret != -5, "add_vertex: breaking multiplicity: return should be -5 but ret=%d", ret);

    ret = graph_add_edge_sw("C", "B", NULL, GRAPH_EDGE_DIRECTED, 2, g);
    fail_if( ret != 0, "add_vertex: return should be 0 but ret=%d", ret);

    //ret = graph_add_vertex(NULL, NULL, g);
    //fail_if( ret < 10, "add_vertex: return should be the new id but id=%d", ret);

    //graph_print(g);

    graph_print_dot("check_graph.gv", g);

    graph_free(NULL, NULL, g);
}
END_TEST


START_TEST(test_cc_grade)
{
    int ret;
    float cc;

    ret = graph_get_vertex_grade_s("A", GRAPH_EDGE_OUT, g);
    fail_if(ret != 2, "grade(A): should be 2, but ret=%d", ret);

    ret = graph_get_vertex_grade_s("C", GRAPH_EDGE_IN, g);
    fail_if(ret != 1, "grade_in(C): should be 1, but ret=%d", ret);
    ret = graph_get_vertex_grade_s("C", GRAPH_EDGE_OUT, g);
    fail_if(ret != 2, "grade_out(C): should be 2, but ret=%d", ret);
    ret = graph_get_vertex_grade_s("C", GRAPH_EDGE_ALL, g);
    fail_if(ret != 3, "grade_all(C): should be 3, but ret=%d", ret);

    geds(C,A,1);
    geds(F,B,1);
    ges(C,B,1);
    //geds(C,D,1);
    ges(A,C,1);

    graph_print_dot("check_graph_cc1.gv", g);
    //graph_print(g);
    
    cc = graph_get_vertex_clustering_coefficient_s ("B", GRAPH_EDGE_DIRECTED, g);
    fail_if(cc != -1, "clustering_coefficient(B): should be -1, but cc=%f", cc);
    cc = graph_get_vertex_clustering_coefficient_s ("C", GRAPH_EDGE_DIRECTED, g);
    fail_if(abs(cc - 1.0/3) > 0.00001, "clustering_coefficient(C): should be 1/3, but cc=%f", cc);
    cc = graph_get_vertex_clustering_coefficient_s ("A", GRAPH_EDGE_ALL, g);
    fail_if(abs(cc - 2.0/3) > 0.00001, "clustering_coefficient(A, dir | non_dir): should be 2/3, but cc=%f", cc);
    cc = graph_get_vertex_clustering_coefficient_s ("A", GRAPH_EDGE_DIRECTED, g);
    fail_if(cc != 0.5, "clustering_coefficient(A, dir): should be 0.5, but cc=%f", cc);
    cc = graph_get_vertex_clustering_coefficient_s ("A", GRAPH_EDGE_NON_DIRECTED, g);
    fail_if(cc != -1, "clustering_coefficient(A, non_dir): should be -1, but cc=%f", cc);

    graph_remove_edge_s ("B","D", GRAPH_EDGE_DIRECTED, NULL, g);
    graph_remove_vertex_s ("E", NULL, NULL, g);
    graph_remove_vertex_s ("g", NULL, NULL, g);
    graph_remove_vertex_s ("h", NULL, NULL, g);

    cc = graph_get_clustering_coefficient (GRAPH_EDGE_ALL, g);
    fail_if(abs(cc - 7.0/45) > 0.00001, "clustering_coefficient(ALL): should be 0.17777, but cc=%f", cc);
    cc = graph_get_clustering_coefficient (GRAPH_EDGE_DIRECTED, g);
    fail_if(abs(cc - 5.0/6) > 0.00001, "clustering_coefficient(DIRECTED): should be 0.16666, but cc=%f", cc);
    cc = graph_get_clustering_coefficient (GRAPH_EDGE_NON_DIRECTED, g);
    fail_if(cc != 0, "clustering_coefficient(NON_DIRECTED): should be 0, but cc=%f", cc);
    graph_print_dot("check_graph_cc2.gv", g);
}
END_TEST

START_TEST(test_bipartiteness)
{
    int i;
    int ret;
    int *distribution;
    geds(E,A,1);
    ges(g,A,1);
    ges(B,C,1);
    graph_remove_edge_s ("C","B", GRAPH_EDGE_DIRECTED, NULL, g);
    graph_remove_vertex_s("A", NULL, NULL, g);

    graph_print_dot("check_graph_bp.gv", g);
    ret = graph_get_bipartiteness (NULL, GRAPH_EDGE_ALL, g);
    fail_if(ret != 0, "bipartiteness(ALL): should be , but ret=%d", ret);
    ret = graph_get_bipartiteness (NULL, GRAPH_EDGE_NON_DIRECTED, g);
    fail_if(ret != 1, "bipartiteness(NON_DIRECTED): should be 1, but ret=%d", ret);
    ret = graph_get_bipartiteness (&distribution, GRAPH_EDGE_DIRECTED, g);
    fail_if(ret != 1, "bipartiteness(DIRECTED): should be 1, but ret=%d", ret);
    printf ("%s:%d i = %d\n", __FILE__, __LINE__, i);    // DEPURACION
    for (i = 0; i < g->num_vertices; i++)
    {
        if (graph_get_vertex_i(i, g) != NULL)
            printf ("%s:%d i: %d, name = %s\n", __FILE__, __LINE__, i, graph_get_vertex_i(i, g)->name);  // DEPURACION
        printf ("%s:%d distribution[%d] = %d\n", __FILE__, __LINE__, i, distribution[i]);    // DEPURACION
    }
    free(distribution);
}
END_TEST


START_TEST(test_dijkstra)
{
    int i;
    graph_path_t * p;

    //graph_print_dot("check_graph_cc.gv", g);
    //graph_print(g);
    printf("about to enter dijkstra\n");
    graph_print_dot_w("Dijkstra.dot", g);
    p = graph_run_dijkstra (graph_get_vertex_s ("A", g), GRAPH_EDGE_DIRECTED, g);

    printf("ID\t\tDist\t\tFather\n");
    for (i = 0; i < graph_get_order(g); i++)
    {
        printf("%d\t\t%f\t\t%d\n", i, p[i].distance, p[i].father);
    }
}
END_TEST

START_TEST(test_disjoint)
{
    array_list_t *l = graph_vertex_disjoint(g);
    fail_if(l->size != 2, "Vertex disjoint Subgraphs = 2\n");
}
END_TEST


START_TEST(test_big_graph) {

}
END_TEST

void print_v(void* vv)
{
    vertex_t *v = (vertex_t*)vv;
    if(vv != NULL)
        //		printf("%p(%s,%d), ",v,v->name, v->id);
        printf("%d, ",v->id);
    else
        printf("NULL, ");
}

START_TEST(test_neighbor_k_level) {
    g = graph_new(GRAPH_MIXED_DIRECTED | GRAPH_CYCLIC |GRAPH_STRICT, 300, COLLECTION_MODE_ASYNCHRONIZED);
    char name[255];

    const int num_vertices = 8;

    int i;

    for(i = 0; i < num_vertices; i++)
    {
        sprintf(name, "%d",i);
        graph_add_vertex(name,NULL, g);
        //printf("Added %s as %d\n",name, graph_add_vertex(name,NULL, g)); 
    }

    ge(0,1);
    ge(0,2);
    ge(1,3);
    ge(2,3);
    ge(1,5);
    ge(5,6);
    ge(5,7);
    ge(7,4);

    linked_list_t *l = graph_get_vertex_neighborhood_i(0,GRAPH_EDGE_ALL, 3, g);
    linked_list_print(l, print_v);

    graph_print_dot("graph_nh.gv",g);
    graph_free(NULL, NULL, g);


}
END_TEST

START_TEST(test_mcl)
{
    graph_t * g2 = graph_mcl(2,2,g);
    graph_print_dot("check_graph_bp.gv", g2);
}
END_TEST

/* ******************************
 *      Main entry point        *
 * ******************************/

int main (int argc, char *argv) {
    Suite *fs = create_test_suite();
    SRunner *fs_runner = srunner_create(fs);
    srunner_run_all(fs_runner, CK_NORMAL);
    int number_failed = srunner_ntests_failed (fs_runner);
    srunner_free (fs_runner);
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}


Suite *create_test_suite(void) {
    TCase *tc_create_free = tcase_create("Create and free");

    //tcase_add_checked_fixture(tc_create_free, create_graph, free_graph);
    tcase_add_test(tc_create_free, test_empty_graph);
    tcase_add_test(tc_create_free, test_small_graph);
    tcase_add_test(tc_create_free, test_big_graph);
    tcase_add_test(tc_create_free, test_neighbor_k_level);
    //tcase_add_test(tc_create_free, test_create_free);

    TCase *tc_profiling = tcase_create("Profiling");
    tcase_add_checked_fixture(tc_profiling, create_graph, free_graph);
    tcase_add_test(tc_profiling, test_cc_grade);
    tcase_add_test(tc_profiling, test_dijkstra);
    tcase_add_test(tc_profiling, test_disjoint);
    tcase_add_test(tc_profiling, test_bipartiteness);
    tcase_add_test(tc_profiling, test_mcl);
    //tcase_add_test(tc_iterators, test_iterators);

    // Add test cases to a test suite
    Suite *fs = suite_create("Graph");
    suite_add_tcase(fs, tc_create_free);
    suite_add_tcase(fs, tc_profiling);

    return fs;
}





/*



#include <stdio.h>
#include <string.h>

#include "../graph/graph.h"
graph_t *g;
void foo()
{


}

void free_cond(void* c)
{
if(c)
free(c);
}

void* new_mem(const char* c)
{
return NULL;
char* v = malloc(20);
int i;
for(i = 0; i < 20 && c[i] != 0; i++)
v[i]=c[i];
v[i] = 0;
return v;
}

int main()
{
printf("Holamundo\n");

g = graph_new(GRAPH_MIXED_DIRECTED | GRAPH_ACYCLIC, 20, COLLECTION_MODE_ASYNCHRONIZED);


graph_add_vertex("A", NULL, g);
graph_add_vertex("B", NULL, g);
graph_add_vertex("C", NULL, g);
graph_add_vertex("D", malloc(29), g);

graph_add_edge_i(0,1, new_mem("Hola"), GRAPH_DIRECTED,2, g);
graph_add_edge_i(1,2, new_mem("Hola"), GRAPH_DIRECTED,2, g);
void* v = malloc(23);
if(graph_add_edge_i(2,0, v, GRAPH_DIRECTED,2, g)<0)
free(v);


graph_print(g);
printf("Clear\n");
graph_clear(free_cond,free_cond,g);

graph_add_vertex("A", NULL, g);
graph_add_vertex("S", NULL, g);
graph_add_vertex("D", NULL, g);
graph_add_vertex("F", NULL, g);
graph_add_vertex("G", NULL, g);

graph_add_edge_s("A","S",NULL,GRAPH_DIRECTED,2,g);
graph_add_edge_s("S","D",NULL,GRAPH_DIRECTED,2,g);
graph_add_edge_s("S","D",NULL,GRAPH_DIRECTED,2,g);
graph_add_edge_s("G","D",NULL,GRAPH_DIRECTED,2,g);
graph_add_edge_s("A","D",NULL,GRAPH_NON_DIRECTED,2,g);
graph_add_edge_s("D","D",NULL,GRAPH_DIRECTED,2,g);
graph_add_edge_s("A","F",NULL,GRAPH_NON_DIRECTED,2,g);


graph_add_edge_s("F","G",new_mem("Pepe"),GRAPH_DIRECTED,2,g);
graph_add_edge_s("F","G",new_mem("Pepon"),GRAPH_DIRECTED,2,g);
graph_add_edge_s("F","G",new_mem("Null"),GRAPH_DIRECTED,2,g);


graph_add_edge_s("G","S",NULL,GRAPH_NON_DIRECTED,9,g);
graph_add_edge_s("F","S",NULL,GRAPH_DIRECTED,2,g);

edge_t * e = graph_get_edge_i(3,4,GRAPH_DIRECTED,g);
if(e != NULL)
    printf("Edge Tomado %s\n",(char*)e->data);
    graph_remove_edge_e(e,GRAPH_DIRECTED,free_cond,g);
    graph_remove_edge_s("A","S",GRAPH_DIRECTED,free_cond,g);
    graph_remove_edge_s("A","Sasdfasd",GRAPH_DIRECTED,free_cond,g);
    graph_remove_edge_s("A","G",GRAPH_DIRECTED,free_cond,g);
    //printf("Edge Eliminado\n");


    //graph_print(g);
    //printf("Go go to Eliminar vertice\n");
    //graph_remove_vertex_s("A",NULL,free_cond,g);

    //graph_add_vertex("GAUSON", NULL, g);
    /*	linked_list_iterator_t *iter = linked_list_iterator_new(l);
        char* c = (char*)linked_list_iterator_curr(iter);
        while(c != NULL)
        {
        printf("CHAR DEL LINKED : %s\n",c);
        c = (char*)linked_list_iterator_next(iter);

        }
        linked_list_iterator_free(iter);
        linked_list_free(l, NULL);
     */
    /*
       graph_add_edge_s("GAUSON", "F", NULL, GRAPH_NON_DIRECTED,3.1415, g);
       graph_add_edge_s("GAUSON", "F", NULL, GRAPH_DIRECTED,2, g);
       graph_add_edge_s("F", "GAUSON", NULL, GRAPH_DIRECTED,1.618, g);


    //graph_remove_vertex_s("dsfg",NULL,free_cond,g);
    //graph_remove_vertex_s("S",NULL,free_cond,g);

    graph_print(g);

    graph_print_dot("grafo.gv",g);

    //printf("Existe camino entre 0 y 3? %d\n", graph_path_exists(0,3,g));

    graph_free(NULL,free_cond,g);

    return 0;
    }

     */
