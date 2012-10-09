#include <sstream>
#include "inventory.h"
#include "game.h"
#include "keypress.h"
#include <algorithm>

item& inventory::operator[] (int i)
{
 if (i < 0 || i > items.size()) {
  debugmsg("Attempted to access item %d in an inventory (size %d)",
           i, items.size());
  return nullitem;
 }

 return items[i][0];
}

item& inventory::stack_item(int i, int j)
{
 if (i < items.size() ) {
  std::vector<item> & stack = items[i];
  if (j < stack.size() ) {
   return stack[j];
  }
  else
   debugmsg("Attempted to access item (%d,%d) in an inventory (stack size %d)",
             i, j, stack.size());
 }
 else
  debugmsg("Attempted to access stack %d in an inventory (size %d)",
           i, items.size());
 return nullitem;
}

void inventory::erase_item(int i, int j)
{
 if (i < items.size() ) {
  std::vector<item> & stack = items[i];
  if (j < stack.size() ) {
   stack.erase(stack.begin()+j);
   return;
  }
  else
   debugmsg("Attempted to erase item (%d,%d) in an inventory (stack size %d)",
             i, j, stack.size());
 }
 else
  debugmsg("Attempted to erase item from stack %d in an inventory (size %d)",
           i, items.size());
}

const std::vector<item> & inventory::stack_at(int i) const
{
 if (i < 0 || i > items.size()) {
  debugmsg("Attempted to access stack %d in an inventory (size %d)",
           i, items.size());
  return nullstack;
 }
 return items[i];
}

const std::vector<item>& inventory::const_stack(int i) const
{
 if (i < 0 || i > items.size()) {
  debugmsg("Attempted to access stack %d in an inventory (size %d)",
           i, items.size());
  return nullstack;
 }
 return items[i];
}

std::vector<item> inventory::as_vector()
{
 std::vector<item> ret;
 for (int i = 0; i < size(); i++) {
  for (int j = 0; j < stack_at(i).size(); j++)
   ret.push_back(items[i][j]);
 }
 return ret;
}

int inventory::size() const
{
 return items.size();
}

int inventory::num_items() const
{
 int ret = 0;
 for (int i = 0; i < items.size(); i++)
  ret += items[i].size();

 return ret;
}

inventory& inventory::operator= (const inventory &rhs)
{
 if (this == &rhs)
  return *this; // No self-assignment

 items = rhs.items;
 weapon_ = rhs.weapon_;
 worn_ = rhs.worn_;
 return *this;
}

inventory& inventory::operator+= (const inventory &rhs)
{
 for (int i = 0; i < rhs.size(); i++)
  add_stack(rhs.const_stack(i));
 return *this;
}

inventory& inventory::operator+= (const std::vector<item> &rhs)
{
 for (int i = 0; i < rhs.size(); i++)
  add_item(rhs[i]);
 return *this;
}

inventory& inventory::operator+= (const item &rhs)
{
 add_item(rhs);
 return *this;
}

inventory inventory::operator+ (const inventory &rhs)
{
 return inventory(*this) += rhs;
}

inventory inventory::operator+ (const std::vector<item> &rhs)
{
 return inventory(*this) += rhs;
}

inventory inventory::operator+ (const item &rhs)
{
 return inventory(*this) += rhs;
}

void inventory::clear()
{
/*
 for (int i = 0; i < items.size(); i++) {
  for (int j = 0; j < items[j].size(); j++)
   delete items[i][j];
 }
*/
 items.clear();
}

void inventory::add_stack(const std::vector<item> & newits)
{
 for (int i = 0; i < newits.size(); i++)
  add_item(newits[i]);
}

void inventory::push_back(const std::vector<item> & newits)
{
 add_stack(newits);
}
 
void inventory::add_item(item newit)
{
 if (newit.is_style())
  return; // Styles never belong in our inventory.

 bool collides = false;

 // Check if stacks
 for( std::vector< std::vector<item> >::iterator stack_it = items.begin(),
      stack_it_end = items.end(); stack_it != stack_it_end; ++stack_it ) {
  for( std::vector<item>::iterator it = stack_it->begin(), end = stack_it->end();
        it != end; ++it ) {
   if (it->stacks_with(newit)) {
    newit.invlet = it->invlet;
    stack_it->push_back(newit);
    return;
   }
   if( it->invlet == newit.invlet )
    collides = true;
  }
 }

 // Use existing letter if possible, else pick a new one.
 if( collides || !newit.invlet_is_okay() )
  assign_empty_invlet(newit);

 // Add new stack
 std::vector<item> newstack;
 newstack.push_back(newit);
 items.push_back(newstack);
}

void inventory::add_item_keep_invlet(const item & newit)
{
 add_item(newit);
}

void inventory::push_back(const item & newit)
{
 add_item(newit);
}

void inventory::restack()
{
 // Move items over to temp location, so we can fill it back up without
 // affecting algorithms like has_weapon_or_armor.
 std::vector< std::vector<item> > tmp;
 tmp.swap( items );

 for ( std::vector< std::vector<item> >::iterator outer_it = tmp.begin(),
   outer_it_end = tmp.end(); outer_it != outer_it_end; ++outer_it ) {
  if (!outer_it->empty()) { // Skip empty
   item & fit = outer_it->at(0);
   if( !fit.invlet_is_okay() || has_weapon_or_armor(fit.invlet)) {
    assign_empty_invlet(fit);
    std::vector<item>::iterator it = outer_it->begin(),
                            it_end = outer_it->end();
    ++it; // skip first we just did
    for( ; it != it_end; ++it ) {
     it->invlet = fit.invlet; // Update rest in stack
    }
   }

   // Now we can move the stack back over
   std::vector<item> empty_stack;
   std::vector< std::vector<item> >::iterator toswap = items.insert(
                                             items.end(), empty_stack );
   outer_it->swap( *toswap );
  }
 }
}

void inventory::form_from_map(game *g, point origin, int range)
{
 items.clear();
 for (int x = origin.x - range; x <= origin.x + range; x++) {
  for (int y = origin.y - range; y <= origin.y + range; y++) {
   for (int i = 0; i < g->m.i_at(x, y).size(); i++)
    if (!g->m.i_at(x, y)[i].made_of(LIQUID))
     add_item(g->m.i_at(x, y)[i]);
// Kludge for now!
   if (g->m.field_at(x, y).type == fd_fire) {
    item fire(g->itypes[itm_fire], 0);
    fire.charges = 1;
    add_item(fire);
   }
  }
 }
}

std::vector<item> inventory::remove_stack(int index)
{
 if (index < 0 || index >= items.size()) {
  debugmsg("Tried to remove_stack(%d) from an inventory (size %d)",
           index, items.size());
  std::vector<item> nullvector;
  return nullvector;
 }
 std::vector<item> ret = stack_at(index);
 items.erase(items.begin() + index);
 return ret;
}

item inventory::remove_item(int index)
{
 if (index < 0 || index >= items.size()) {
  debugmsg("Tried to remove_item(%d) from an inventory (size %d)",
           index, items.size());
  return nullitem;
 }

 item ret = items[index][0];
 items[index].erase(items[index].begin());
 if (items[index].empty())
  items.erase(items.begin() + index);

 return ret;
}

item inventory::remove_item(int stack, int index)
{
 if (stack < 0 || stack >= items.size()) {
  debugmsg("Tried to remove_item(%d, %d) from an inventory (size %d)",
           stack, index, items.size());
  return nullitem;
 } else if (index < 0 || index >= items[stack].size()) {
  debugmsg("Tried to remove_item(%d, %d) from an inventory (stack is size %d)",
           stack, index, items[stack].size());
  return nullitem;
 }

 item ret = items[stack][index];
 items[stack].erase(items[stack].begin() + index);
 if (items[stack].empty())
  items.erase(items.begin() + stack);

 return ret;
}

item inventory::remove_item_by_letter(char ch)
{
 for (int i = 0; i < items.size(); i++) {
  if (items[i][0].invlet == ch) {
   if (items[i].size() > 1)
    items[i][1].invlet = ch;
   return remove_item(i);
  }
 }

 return nullitem;
}

item& inventory::item_by_letter(char ch)
{
 for (int i = 0; i < items.size(); i++) {
  if (items[i][0].invlet == ch)
   return items[i][0];
 }
 return nullitem;
}

int inventory::index_by_letter(char ch)
{
 if (ch == KEY_ESCAPE)
  return -1;
 for (int i = 0; i < items.size(); i++) {
  if (items[i][0].invlet == ch)
   return i;
 }
 return -1;
}

int inventory::amount_of(itype_id it)
{
 int count = 0;
 for (int i = 0; i < items.size(); i++) {
  for (int j = 0; j < items[i].size(); j++) {
   if (items[i][j].type->id == it)
    count++;
   for (int k = 0; k < items[i][j].contents.size(); k++) {
    if (items[i][j].contents[k].type->id == it)
     count++;
   }
  }
 }
 return count;
}

int inventory::charges_of(itype_id it)
{
 int count = 0;
 for (int i = 0; i < items.size(); i++) {
  for (int j = 0; j < items[i].size(); j++) {
   if (items[i][j].type->id == it) {
    if (items[i][j].charges < 0)
     count++;
    else
     count += items[i][j].charges;
   }
   for (int k = 0; k < items[i][j].contents.size(); k++) {
    if (items[i][j].contents[k].type->id == it) {
     if (items[i][j].contents[k].charges < 0)
      count++;
     else
      count += items[i][j].contents[k].charges;
    }
   }
  }
 }
 return count;
}

void inventory::use_amount(itype_id it, int quantity, bool use_container)
{
 for (int i = 0; i < items.size() && quantity > 0; i++) {
  for (int j = 0; j < items[i].size() && quantity > 0; j++) {
// First, check contents
   bool used_item_contents = false;
   for (int k = 0; k < items[i][j].contents.size() && quantity > 0; k++) {
    if (items[i][j].contents[k].type->id == it) {
     quantity--;
     items[i][j].contents.erase(items[i][j].contents.begin() + k);
     k--;
     used_item_contents = true;
    }
   }
// Now check the item itself
   if (use_container && used_item_contents) {
    items[i].erase(items[i].begin() + j);
    j--;
    if (items[i].empty()) {
     items.erase(items.begin() + i);
     i--;
     j = 0;
    }
   } else if (items[i][j].type->id == it && quantity > 0) {
    quantity--;
    items[i].erase(items[i].begin() + j);
    j--;
    if (items[i].empty()) {
     items.erase(items.begin() + i);
     i--;
     j = 0;
    }
   }
  }
 }
}

void inventory::use_charges(itype_id it, int quantity)
{
 for (int i = 0; i < items.size() && quantity > 0; i++) {
  for (int j = 0; j < items[i].size() && quantity > 0; j++) {
// First, check contents
   for (int k = 0; k < items[i][j].contents.size() && quantity > 0; k++) {
    if (items[i][j].contents[k].type->id == it) {
     if (items[i][j].contents[k].charges <= quantity) {
      quantity -= items[i][j].contents[k].charges;
      if (items[i][j].contents[k].destroyed_at_zero_charges()) {
       items[i][j].contents.erase(items[i][j].contents.begin() + k);
       k--;
      } else
       items[i][j].contents[k].charges = 0;
     } else {
      items[i][j].contents[k].charges -= quantity;
      return;
     }
    }
   }
// Now check the item itself
   if (items[i][j].type->id == it) {
    if (items[i][j].charges <= quantity) {
     quantity -= items[i][j].charges;
     if (items[i][j].destroyed_at_zero_charges()) {
      items[i].erase(items[i].begin() + j);
      j--;
      if (items[i].empty()) {
       items.erase(items.begin() + i);
       i--;
       j = 0;
      }
     } else
      items[i][j].charges = 0;
    } else {
     items[i][j].charges -= quantity;
     return;
    }
   }
  }
 }
}
 
bool inventory::has_amount(itype_id it, int quantity)
{
 return (amount_of(it) >= quantity);
}

bool inventory::has_charges(itype_id it, int quantity)
{
 return (charges_of(it) >= quantity);
}

bool inventory::has_item(item *it)
{
 for (int i = 0; i < items.size(); i++) {
  for (int j = 0; j < items[i].size(); j++) {
   if (it == &(items[i][j]))
    return true;
   for (int k = 0; k < items[i][j].contents.size(); k++) {
    if (it == &(items[i][j].contents[k]))
     return true;
   }
  }
 }
 return false;
}

item& inventory::weapon()
{
 return weapon_;
}

const item& inventory::weapon() const
{
 return weapon_;
}

void inventory::set_weapon(const item & w)
{
 // TODO: Make sure inventory letter sane.
 weapon_ = w;
}

const std::vector<item>& inventory::worn_items() const
{
 return worn_;
}

item& inventory::worn_item_at(int i)
{
 return worn_[i];
}

void inventory::remove_worn_item(int i)
{
 worn_.erase(worn_.begin() + i);
}

void inventory::remove_worn_items(std::vector<int> item_ids)
{
 if(item_ids.empty()) return;

 // Numbers most likely to be sorted in order already, but just in case:
 std::sort(item_ids.begin(), item_ids.end());
 std::vector<int>::iterator nums = item_ids.begin(),
  nums_end = item_ids.end();

 int current_place = 0;
 for(std::vector<item>::iterator it = worn_.begin();
      it != worn_.end() && nums != nums_end; ++current_place ) {
  if( current_place == *nums ) {
   ++nums; // Fount this one.
   it = worn_.erase(it);
  }
  else
   ++it;
 }
}

void inventory::add_worn_item(const item & it)
{
 // TODO: Fix inv id
 worn_.push_back(it);
}

void inventory::clear_worn_items()
{
 worn_.clear();
}

bool inventory::has_weapon_or_armor(char let)
{
 if (weapon_.invlet == let)
  return true;

 for( std::vector<item>::iterator it = worn_.begin(),
             it_end = worn_.end(); it != it_end; ++it )
  if (it->invlet == let)
   return true;

 return false;
}

bool inventory::give_inventory_letter(item & newit)
{
 if (newit.is_style())
  return false; // Styles never belong in our inventory.

 bool collides = false;
 char orig_char = newit.invlet;

 // Check if stacks
 for( std::vector< std::vector<item> >::iterator stack_it = items.begin(),
      stack_it_end = items.end(); stack_it != stack_it_end; ++stack_it ) {
  for( std::vector<item>::iterator it = stack_it->begin(), end = stack_it->end();
        it != end; ++it ) {
   if (it->stacks_with(newit)) {
    newit.invlet = it->invlet;
    return true;
   }
   if( it->invlet == newit.invlet )
   {
    collides = true;
   }
  }
 }

 // Use existing letter if possible, else pick a new one.
 if( collides || !newit.invlet_is_okay() )
 {
  assign_empty_invlet(newit);
 }

 if( newit.invlet == '`' ) {
  newit.invlet = orig_char;
  return false;
 }
 return true;
}

void inventory::assign_empty_invlet(item &it)
{
 for (int ch = 'a'; ch <= 'z'; ch++) {
  //debugmsg("Trying %c", ch);
  if (index_by_letter(ch) == -1 && !has_weapon_or_armor(ch)) {
   it.invlet = ch;
   //debugmsg("Using %c", ch);
   return;
  }
 }
 for (int ch = 'A'; ch <= 'Z'; ch++) {
  //debugmsg("Trying %c", ch);
  if (index_by_letter(ch) == -1 && !has_weapon_or_armor(ch)) {
   //debugmsg("Using %c", ch);
   it.invlet = ch;
   return;
  }
 }
 it.invlet = '`';
 //debugmsg("Couldn't find empty invlet");
}
