#ifndef _INVENTORY_H_
#define _INVENTORY_H_

#include "item.h"
#include <string>
#include <vector>

class game;
class map;

class inventory
{
 public:
  item& operator[] (int i);
  const std::vector<item>& stack_at(int i) const;
  const std::vector<item>& const_stack(int i) const;
  std::vector<item> as_vector();
  item& front();
  item& back();
  int size() const;
  int num_items() const;
  item& stack_item(int i, int j);

  inventory& operator=  (const inventory &rhs);
  inventory& operator+= (const inventory &rhs);
  inventory& operator+= (const item &rhs);
  inventory& operator+= (const std::vector<item> &rhs);
  inventory  operator+  (const inventory &rhs);
  inventory  operator+  (const item &rhs);
  inventory  operator+  (const std::vector<item> &rhs);

  void clear();
  void add_stack(const std::vector<item> & newits);
  void push_back(const std::vector<item> & newits);
  void push_back(const item & newit);
  void add_item_keep_invlet(const item & newit);
  void add_item (item newit);
  bool give_inventory_letter(item &);

/* Check all items for proper stacking, rearranging as needed
 * game pointer is not necessary, but if supplied, will ensure no overlap with
 * the player's worn items / weapon
 */
  void restack();

  void form_from_map(game *g, point origin, int distance);

  std::vector<item> remove_stack(int index);
  void  erase_item(int i, int j);
  item  remove_item(int index);
  item  remove_item(int stack, int index);
  item  remove_item_by_letter(char ch);
  item& item_by_letter(char ch);
  int   index_by_letter(char ch);

// Below, "amount" refers to quantity
//        "charges" refers to charges
  int  amount_of (itype_id it);
  int  charges_of(itype_id it);

  void use_amount (itype_id it, int quantity, bool use_container = false);
  void use_charges(itype_id it, int quantity);

  bool has_amount (itype_id it, int quantity);
  bool has_charges(itype_id it, int quantity);
  bool has_item(item *it); // Looks for a specific item

  const item& weapon() const;
  item& weapon();
  void set_weapon(const item &);

/* TODO: This stuff, I guess?
  std::string save();
  void load(std::string data);
*/

  item nullitem;
  std::vector<item> nullstack;
  const std::vector<item> & worn_items() const;
  void remove_worn_item(int i);
  void remove_worn_items(std::vector<int>);
  item& worn_item_at(int);
  void add_worn_item(item);
  void clear_worn_items();

  bool has_weapon_or_armor(char let);

 private:
  void assign_empty_invlet(item &it);
  std::vector< std::vector<item> > items;
  item weapon_;
  std::vector <item> worn_;
};

#endif
