// Copyright 2004 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Andrew Lumsdaine
#include <boost/graph/fruchterman_reingold.hpp>
#include <boost/graph/kamada_kawai_spring_layout.hpp>
#include <boost/graph/random_layout.hpp>
#include <boost/graph/circle_layout.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/topology.hpp>
#include <boost/lexical_cast.hpp>
#include <string>
#include <iostream>
#include <map>
#include <vector>
#include <boost/random/linear_congruential.hpp>
#include <boost/timer/progress_display.hpp>
#include <boost/shared_ptr.hpp>

#include "layout.h"

using namespace boost;



void usage()
{
    std::cerr << "Usage: fr_layout [options] <width> <height>\n"
              << "Arguments:\n"
              << "\t<width>\tWidth of the display area (floating point)\n"
              << "\t<Height>\tHeight of the display area (floating point)\n\n"
              << "Options:\n"
              << "\t--iterations n\tNumber of iterations to execute.\n"
              << "\t\t\tThe default value is 100.\n"
              << "Input:\n"
              << "  Input is read from standard input as a list of edges, one "
                 "per line.\n"
              << "  Each edge contains two string labels (the endpoints) "
                 "separated by a space.\n\n"
              << "Output:\n"
              << "  Vertices and their positions are written to standard "
                 "output with the label,\n  x-position, and y-position of a "
                 "vertex on each line, separated by spaces.\n";
}


typedef boost::rectangle_topology<> topology_type;
typedef topology_type::point_type point_type;

typedef adjacency_list< /*listS*/setS, vecS, undirectedS,
			property< vertex_name_t, std::string >,
			property< edge_index_t, int >                     
			// 	  property< edge_weight_t, double > >
			>
    Graph;

typedef graph_traits< Graph >::vertex_descriptor Vertex;
typedef graph_traits< Graph >::edge_descriptor Edge;

typedef std::map< std::string, Vertex > NameToVertex;

typedef std::vector< point_type > PositionVec;

typedef iterator_property_map< PositionVec::iterator,
			       property_map< Graph, vertex_index_t >::type >
PositionMap;

typedef iterator_property_map< std::vector< double >::iterator,
			       property_map< Graph, edge_index_t >::type >
WMap;


Vertex get_vertex(const std::string& name, Graph& g, NameToVertex& names)
{
    NameToVertex::iterator i = names.find(name);
    if (i == names.end())
        i = names.insert(std::make_pair(name, add_vertex(name, g))).first;
    return i->second;
}

class progress_cooling : public linear_cooling< double >
{
    typedef linear_cooling< double > inherited;

public:
  explicit progress_cooling(std::size_t iterations, layout_update_cb_t f ) : inherited(iterations)
    {
        display.reset(new boost::timer::progress_display(iterations + 1, std::cerr));
	iter=0;
	cb = f;
    }

    double operator()()
    {
        ++(*display);
	iter += 1;
	cb( iter, 10000 );
        return inherited::operator()();
    }

private:
  shared_ptr< boost::timer::progress_display > display;
  int iter;
  layout_update_cb_t cb;
};


void layout_open( void ** graph, void ** labels, void ** weights ){
  Graph *g			= new Graph;
  NameToVertex *names		= new NameToVertex;
  std::vector<double> *w	= new std::vector<double>;
  *graph			= (void *) g;
  *labels			= (void *) names;
  *weights                      = (void *) w;
}

void layout_close( void *g, void *names, void *pos, void *weights ){
  if( g )	delete (Graph *) g;
  if( names )	delete (NameToVertex *) names;
  if( pos )	delete (PositionMap *) pos;
  if( weights ) delete (std::vector<double> *) weights;
}

void layout_add_edge( void *graph, void *labels, void *weights,
		      char *s, char *t, int n, double wedge ){
  Graph *g			= (Graph *) graph;
  NameToVertex *names		= (NameToVertex *) labels;
  std::vector<double> *w	= (std::vector<double> *) weights;

  // std::string source(s), target(t);
  Edge edge;
  bool b;
  boost::tie( edge, b ) =  add_edge(get_vertex( std::string(s), *g, *names ),
				    get_vertex( std::string(t), *g, *names ), n, *g );
  (*w).push_back( wedge );
  printf( "Adding Edge (%d): %s %s, weight=%f (size=%d)\n", b, (char *) get( vertex_name, *g, source( edge, *g ) ).c_str(),
	  (char *) get( vertex_name, *g, target( edge, *g ) ).c_str(), wedge, (int) (*w).size()  );
}

// REDRAWING
layout_update_cb_t S_cb = NULL;
int S_iter		= 0;

bool kk_done( double delta_p, Vertex p, Graph g, bool maxp ){
  layout_tolerance< double > f;
  // printf( "Done %f, vertex=%s (max=%d)\n", delta_p,  (char *) get(vertex_name, g, p).c_str(), maxp );
  S_cb( S_iter++, 100 );
  return delta_p < 10. ? true : false ;
}

void layout_run_kk( void *graph, void **pos, void * weights, double width, double height, layout_update_cb_t f ){
  bool b;
  Graph *g			= (Graph *) graph;
  std::vector<double> *w        = (std::vector<double> *) weights;
  PositionVec *position_vec	= new PositionVec(num_vertices(*g));
  PositionMap *position		= new PositionMap((*position_vec).begin(), get(vertex_index, *g));
  *pos = (void *) position;

  minstd_rand gen;
  topology_type topo(gen, -width / 2, -height / 2, width / 2, height / 2);
  circle_graph_layout( *g, *position, (double) width / 2.0 );
  S_cb		= f;
  S_iter	= 0;
  WMap w_map = WMap( (*w).begin(), get( edge_index, *g ) );
  b = kamada_kawai_spring_layout( *g, *position, w_map, topo,
				  boost::side_length( (double) width/2.0 ), kk_done );
  S_cb		= NULL;
  S_iter	= 0;
}

void layout_run_fr( void *graph, void **pos, int iterations, double width, double height, layout_update_cb_t f ){
  Graph *g			= (Graph *) graph;
  PositionVec *position_vec	= new PositionVec(num_vertices(*g));
  PositionMap *position		= new PositionMap((*position_vec).begin(), get(vertex_index, *g));
  *pos = (void *) position;

  minstd_rand gen;
  topology_type topo(gen, -width / 2, -height / 2, width / 2, height / 2);
  random_graph_layout(*g, *position, topo);
  fruchterman_reingold_force_directed_layout( *g, *position, topo, cooling(progress_cooling(iterations, f)));
  //
  //
  // graph_traits< Graph >::vertex_iterator vi, vi_end;
  // for (boost::tie(vi, vi_end) = vertices( *g ); vi != vi_end; ++vi)
  //   {
  //     std::cout << get(vertex_name, *g, *vi) << '\t' << (*position)[*vi][0] << '\t' << typeid( (*position)[*vi][0] ).name() 
  // 		<< '\t' << (*position)[*vi][1] << std::endl;
  //   }
  
}

void layout_enumerate_vertices( void *graph, void *labels, void *pos, layout_enumv_cb_t f ){
  Graph *g = (Graph *) graph;
  PositionMap *position = (PositionMap *) pos;
  graph_traits< Graph >::vertex_iterator vi, vi_end;
  for (boost::tie(vi, vi_end) = vertices( *g ); vi != vi_end; ++vi)
    {
      f( (char *) get(vertex_name, *g, *vi).c_str(),  (*position)[*vi][0],  (*position)[*vi][1] );
      // std::cout << get(vertex_name, *g, *vi) << '\t' << (*position)[*vi][0]
      // 		<< '\t' << (*position)[*vi][1] << std::endl;
    }
}

void layout_enumerate_edges( void *graph, void *labels, void *pos, layout_enume_cb_t f ){
  Graph *g = (Graph *) graph;
  PositionMap *position = (PositionMap *) pos;
  graph_traits< Graph >::edge_iterator ei, ei_end;
  for (boost::tie(ei, ei_end) = edges( *g ); ei != ei_end; ++ei){
    f( (char *) get( vertex_name, *g, source( *ei, *g ) ).c_str(),
       (*position)[source( *ei, *g )][0],
       (*position)[source( *ei, *g )][1],
       (char *) get( vertex_name, *g, target( *ei, *g ) ).c_str(),
       (*position)[target( *ei, *g )][0],
       (*position)[target( *ei, *g )][1] );
  }
}

void *layout_invertex_p( void *graph, void *labels, void *pos, double x, double y ){
  Graph *g = (Graph *) graph;
  PositionMap *position = (PositionMap *) pos;
  graph_traits< Graph >::vertex_iterator vi, vi_end;
  for (boost::tie(vi, vi_end) = vertices( *g ); vi != vi_end; ++vi){
    // printf( "INVERTEXP %s Test x=%f, y=%f\n", (char *) get( vertex_name, *g, *vi ).c_str(),
    // 	    (*position)[*vi][0], (*position)[*vi][1] );
    if( (((*position)[*vi][0] - _MARK_SIZE) <= x) &&
	(((*position)[*vi][0] + _MARK_SIZE) >= x) &&
	(((*position)[*vi][1] - _MARK_SIZE) <= y) &&
	(((*position)[*vi][1] + _MARK_SIZE) >= y) ){
      return (void *) get( vertex_name, *g, *vi ).c_str() ;
    }
  }
  return NULL;
}

int layout_adjacent_p( void *graph, void *labels, void *pos,
		       char *node, char *selected ){
  Graph *g		= (Graph *) graph;
  NameToVertex *names	= (NameToVertex *) labels;
  Vertex vSelected	= get_vertex( std::string(selected), *g, *names ),
    vNode		= get_vertex( std::string(node), *g, *names );
  typename graph_traits<Graph>::adjacency_iterator ai;
  typename graph_traits<Graph>::adjacency_iterator ai_end;
  for (boost::tie(ai, ai_end) = adjacent_vertices(vSelected, *g);
       ai != ai_end; ++ai){
    if( vNode == *ai )
      return 1;
  }

  return 0;
}
