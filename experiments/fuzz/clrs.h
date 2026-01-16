#pragma once

enum Color { RED, BLACK };

struct Node {
    int data;
    Color color;
    Node* left;
    Node* right;
    Node* parent;
};

class RedBlackTree {
public:
    void insert(int val) {
        Node* newNode = new Node(val);
        Node* y = nullptr;
        Node* x = root;

        while (x != nullptr) {
            y = x;
            if (newNode->data < x->data) x = x->left;
            else x = x->right;
        }

        newNode->parent = y;
        if (y == nullptr) root = newNode;
        else if (newNode->data < y->data) y->left = newNode;
        else y->right = newNode;

        fix_insert(newNode);
    }

    void remove(int val) {
        Node* z = root;
        while (z != nullptr) {
            if (val < z->data) z = z->left;
            else if (val > z->data) z = z->right;
            else {
                delete_node(z);
                return;
            }
        }
    }

private:
    Node* root = nullptr;

    void left_rotate(Node* x) {
        if (x == nullptr || x->right == nullptr) return;

        Node* y = x->right;
        x->right = y->left;
        if (y->left != nullptr) y->left->parent = x;
        y->parent = x->parent;
        if (x->parent == nullptr) root = y;
        else if (x == x->parent->left) x->parent->left = y;
        else x->parent->right = y;
        y->left = x;
        x->parent = y;
    }

    void right_rotate(Node* y) {
        if (y == nullptr || y->left == nullptr) return;

        Node* x = y->left;
        y->left = x->right;
        if (x->right != nullptr) x->right->parent = y;
        x->parent = y->parent;
        if (y->parent == nullptr) root = x;
        else if (y == y->parent->left) y->parent->left = x;
        else y->parent->right = x;
        x->right = y;
        y->parent = x;
    }

    void fix_insert(Node* z) {
        while (z != root && z->parent->color == RED) {
            if (z->parent == z->parent->parent->left) {
                Node* y = z->parent->parent->right;
                if (y != nullptr && y->color == RED) {
                    z->parent->color = BLACK;
                    y->color = BLACK;
                    z->parent->parent->color = RED;
                    z = z->parent->parent;
                } else {
                    if (z == z->parent->right) {
                        z = z->parent;
                        left_rotate(z);
                    }
                    z->parent->color = BLACK;
                    z->parent->parent->color = RED;
                    right_rotate(z->parent->parent);
                }
            } else {
                Node* y = z->parent->parent->left;
                if (y != nullptr && y->color == RED) {
                    z->parent->color = BLACK;
                    y->color = BLACK;
                    z->parent->parent->color = RED;
                    z = z->parent->parent;
                } else {
                    if (z == z->parent->left) {
                        z = z->parent;
                        right_rotate(z);
                    }
                    z->parent->color = BLACK;
                    z->parent->parent->color = RED;
                    left_rotate(z->parent->parent);
                }
            }
        }
        root->color = BLACK;
    }

    void transplant(Node* u, Node* v) {
        if (u->parent == nullptr) root = v;
        else if (u == u->parent->left) u->parent->left = v;
        else u->parent->right = v;
        if (v != nullptr) v->parent = u->parent;
    }

    void delete_node(Node* z) {
        if (z == nullptr) return;

        Node* y = z;
        Node* x = nullptr;
        Color y_original_color = y->color;

        if (z->left == nullptr) {
            x = z->right;
            transplant(z, z->right);
        } else if (z->right == nullptr) {
            x = z->left;
            transplant(z, z->left);
        } else {
            y = minimum(z->right);
            y_original_color = y->color;
            x = y->right;

            if (y->parent == z) {
                if (x != nullptr) x->parent = y;
            } else {
                if (x != nullptr) x->parent = y->parent;

                transplant(y, y->right);
                if (y->right != nullptr) y->right->parent = y;
                y->right = z->right;
                if (y->right != nullptr) y->right->parent = y;
            }
            transplant(z, y);
            y->left = z->left;
            if (y->left != nullptr) y->left->parent = y;
            y->color = z->color;
        }

        if (y_original_color == BLACK && x != nullptr) fix_delete(x);

        delete z;
    }

    void fix_delete(Node* x) {
        while (x != root && x != nullptr && x->color == BLACK) {
            if (x == x->parent->left) {
                Node* w = x->parent->right;
                if (w->color == RED) {
                    w->color = BLACK;
                    x->parent->color = RED;
                    left_rotate(x->parent);
                    w = x->parent->right;
                }
                if ((w->left == nullptr || w->left->color == BLACK) &&
                    (w->right == nullptr || w->right->color == BLACK)) {
                    w->color = RED;
                    x = x->parent;
                } else {
                    if (w->right == nullptr || w->right->color == BLACK) {
                        if (w->left != nullptr) w->left->color = BLACK;
                        w->color = RED;
                        right_rotate(w);
                        w = x->parent->right;
                    }
                    w->color = x->parent->color;
                    x->parent->color = BLACK;
                    if (w->right != nullptr) w->right->color = BLACK;
                    left_rotate(x->parent);
                    x = root;
                }
            } else {
                Node* w = x->parent->left;
                if (w->color == RED) {
                    w->color = BLACK;
                    x->parent->color = RED;
                    right_rotate(x->parent);
                    w = x->parent->left;
                }
                if ((w->right == nullptr || w->right->color == BLACK) &&
                    (w->left == nullptr || w->left->color == BLACK)) {
                    w->color = RED;
                    x = x->parent;
                } else {
                    if (w->left == nullptr || w->left->color == BLACK) {
                        if (w->right != nullptr) w->right->color = BLACK;
                        w->color = RED;
                        left_rotate(w);
                        w = x->parent->left;
                    }
                    w->color = x->parent->color;
                    x->parent->color = BLACK;
                    if (w->left != nullptr) w->left->color = BLACK;
                    right_rotate(x->parent);
                    x = root;
                }
            }
        }
        if (x != nullptr) x->color = BLACK;
    }

    Node* minimum(Node* node) {
        while (node->left != nullptr) node = node->left;
        return node;
    }
};
