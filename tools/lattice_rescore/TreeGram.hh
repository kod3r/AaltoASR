#ifndef TREEGRAM_HH
#define TREEGRAM_HH

#include <vector>
#include <deque>
#include <stdio.h>

#include "Vocabulary.hh"

#ifdef USE_CL
template <typename KT, typename CT> class ClusterMap;
#endif

class TreeGram : public Vocabulary {
public:
  typedef std::deque<int> Gram;

  struct Node {
    Node() : word(-1), log_prob(0), back_off(0), child_index(-1) {}
    Node(int word, float log_prob, float back_off, int child_index)
      : word(word), log_prob(log_prob), back_off(back_off), 
	child_index(child_index) {}
    int word;
    float log_prob;
    float back_off;
    int child_index;
  };

  class Iterator {
  public:
    Iterator(TreeGram *gram = NULL);
    void reset(TreeGram *gram);

    // Move to the next node in depth-first order
    bool next();

    // Move to the next node on the given order
    bool next_order(int order);

    // Return the node from the index stack. (default: the last one)
    const Node &node(int order = 0);

    // Order of the current node (1 ... n)
    int order() { return m_index_stack.size(); }

    // Move to within current context (default: to the next word)
    bool move_in_context(int delta = 1);

    // Come back up to previous order
    bool up();

    // Dive down to first child
    bool down();

    friend class TreeGram;
  private:
    TreeGram *m_gram;
    std::vector<int> m_index_stack;
  };

  enum Type { BACKOFF=0, INTERPOLATED=1 };

  TreeGram();
  void set_type(Type type) { m_type = type; }
  Type get_type() { return(m_type); }
  void reserve_nodes(int nodes); 
  void set_interpolation(const std::vector<float> &interpolation);

  /// Adds a new gram to the language model.
  // 
  // The grams must be inserted in sorted order.  The only exception
  // is the OOV 1-gram, which can be updated any time.  It exists by
  // default with very small log-prob and zero back-off.
  void add_gram(const Gram &gram, float log_prob, float back_off);
  void read(FILE *file);
  void write(FILE *file, bool reflip);

  void fetch_bigram_list(int prev_word_id, std::vector<int> &next_word_id,
                         std::vector<float> &result_buffer);
  void fetch_trigram_list(int w1, int w2, std::vector<int> &next_word_id,
                          std::vector<float> &result_buffer);
  
  float log_prob(const Gram &gram);
  int order() { return m_order; }
  int last_order() { return m_last_order; }

  // The history length used in the last log_prob() call, used by
  // Rescore class.  Actually, it is not exactly the history length,
  // the maximum history length for the next call, when one word has
  // been added to history.
  //
  // Examples:
  // * (a b c d) asked and (a b c d) found: length = 4
  // * (a b c d) asked and backoff (a b c) found: length = 3
  // * (a b c d) asked, (a b c) not found, (b c d) found: length 3
  int last_history_length() { return m_last_history_length; }
  int gram_count(int order) { return m_order_count.at(order-1); }

  /* Don't use this function, unles you really need to*/
  int find_child(int word, int node_index);

  // Returns an iterator for given gram.
  Iterator iterator(const Gram &gram);

#ifdef USE_CL
  ClusterMap<int, int> *clmap;
#endif

private:
  int binary_search(int word, int first, int last);
  void print_gram(FILE *file, const Gram &gram);
  void find_path(const Gram &gram);
  void check_order(const Gram &gram);
  void flip_endian();
  void fetch_gram(const Gram &gram, int first);

  Type m_type;
  int m_order;
  std::vector<int> m_order_count;	// number of grams in each order
  std::vector<float> m_interpolation;	// interpolation weights
  std::vector<Node> m_nodes;		// storage for the nodes
  std::vector<int> m_fetch_stack;	// indices of the gram requested
  int m_last_order;			// order of the last hit
  int m_last_history_length;		// length of the history used in
					// hit or backoff

  // For creating the model
  std::vector<int> m_insert_stack;	// indices of the last gram inserted
  Gram m_last_gram;			// the last ngram added to the model
};

#endif /* TREEGRAM_HH */
