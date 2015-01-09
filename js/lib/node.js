"use strict";

function isContainer(node) {
    var t = node.t;
    return (t === 'Document' ||
            t === 'BlockQuote' ||
            t === 'List' ||
            t === 'ListItem' ||
            t === 'Paragraph' ||
            t === 'Header' ||
            t === 'Emph' ||
            t === 'Strong' ||
            t === 'Link' ||
            t === 'Image');
}

function NodeWalker(root) {
    this.current = root;
    this.root = root;
    this.entering = true;
}

NodeWalker.prototype.resumeAt = function(node, entering) {
    this.current = node;
    this.entering = (entering === true);
};

NodeWalker.prototype.next = function(){
    var cur = this.current;
    var entering = this.entering;

    if (!cur) {
        return null;
    }

    var container = isContainer(cur);

    if (entering && container) {
        if (cur.firstChild) {
            this.current = cur.firstChild;
            this.entering = true;
        } else {
            // stay on node but exit
            this.entering = false;
        }

    } else if (!entering && cur === this.root) {
        // don't move past root
        this.current = null;

    } else if (cur.next) {
        this.current = cur.next;
        this.entering = true;

    } else {
        this.current = cur.parent;
        this.entering = false;
    }

    return {entering: entering, node: cur};
};

function Node(nodeType, pos) {
    this.t = nodeType;
    this.parent = null;
    this.firstChild = null;
    this.lastChild = null;
    this.prev = null;
    this.next = null;
    this.pos = pos || {};
    this.last_line_blank = false;
    this.open = true;
    this.strings = [];
    this.string_content = "";
    this.c = undefined;
    this.list_data = undefined;
    this.tight = undefined;
    this.info = undefined;
}

Node.prototype.isContainer = function() {
    return isContainer(this);
};

Node.prototype.appendChild = function(child) {
    child.unlink();
    child.parent = this;
    if (this.lastChild) {
        this.lastChild.next = child;
        child.prev = this.lastChild;
        this.lastChild = child;
    } else {
        this.firstChild = child;
        this.lastChild = child;
    }
};

Node.prototype.prependChild = function(child) {
    child.unlink();
    child.parent = this;
    if (this.firstChild) {
        this.firstChild.prev = child;
        child.next = this.firstChild;
        this.firstChild = child;
    } else {
        this.firstChild = child;
        this.lastChild = child;
    }
};

Node.prototype.unlink = function() {
    if (this.prev) {
        this.prev.next = this.next;
    } else if (this.parent) {
        this.parent.firstChild = this.next;
    }
    if (this.next) {
        this.next.prev = this.prev;
    } else if (this.parent) {
        this.parent.lastChild = this.prev;
    }
    this.parent = null;
    this.next = null;
    this.prev = null;
};

Node.prototype.insertAfter = function(sibling) {
    sibling.unlink();
    sibling.next = this.next;
    if (sibling.next) {
        sibling.next.prev = sibling;
    }
    sibling.prev = this;
    this.next = sibling;
    sibling.parent = this.parent;
    if (!sibling.next) {
        sibling.parent.lastChild = sibling;
    }
};

Node.prototype.insertBefore = function(sibling) {
    sibling.unlink();
    sibling.prev = this.prev;
    if (sibling.prev) {
        sibling.prev.next = sibling;
    }
    sibling.next = this;
    this.prev = sibling;
    sibling.parent = this.parent;
    if (!sibling.prev) {
        sibling.parent.firstChild = sibling;
    }
};

Node.prototype.walker = function() {
    var walker = new NodeWalker(this);
    return walker;
};

Node.prototype.toAST = function() {
    var children;
    var cur;
    var result = { t: this.t };

    var propsToShow = ['t', 'c', 'list_data', 'string_content',
                       'pos', 'tight', 'info'];

    for (var i = 0; i < propsToShow.length; i++) {
        var prop = propsToShow[i];
        if (this[prop] !== undefined) {
            result[prop] = this[prop];
        }
    }

    if (isContainer(this)) {
        children = [];
        if (this.firstChild) {
            cur = this.firstChild;
            while (cur) {
                // TODO avoid recursion here...
                children.push(cur.toAST());
                cur = cur.next;
            }
            result.children = children;
        }
    }
    return result;

};

module.exports = Node;


/* Example of use of walker:

 var walker = w.walker();
 var event;

 while (event = walker.next()) {
 console.log(event.entering, event.node.t);
 }

 */
