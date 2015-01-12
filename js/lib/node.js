"use strict";

function isContainer(node) {
    switch (node.t) {
    case 'Document':
    case 'BlockQuote':
    case 'List':
    case 'ListItem':
    case 'Paragraph':
    case 'Header':
    case 'Emph':
    case 'Strong':
    case 'Link':
    case 'Image':
        return true;
    default:
        return false;
    }
}

var resumeAt = function(node, entering) {
    this.current = node;
    this.entering = (entering === true);
};

var next = function(){
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

    } else if (cur.next) {
        this.current = cur.next;
        this.entering = true;

    } else {
        this.current = cur.parent;
        this.entering = false;
    }

    return {entering: entering, node: cur};
};

var NodeWalker = function(root) {
    return { current: root,
             root: root,
             entering: true,
             next: next,
             resumeAt: resumeAt };
};

var Node = function(nodeType, sourcepos) {
    this.t = nodeType;
    this.parent = null;
    this.firstChild = null;
    this.lastChild = null;
    this.prev = null;
    this.next = null;
    this.sourcepos = sourcepos;
    this.last_line_blank = false;
    this.open = true;
    this.strings = undefined;
    this.string_content = undefined;
    this.literal = undefined;
    this.list_data = undefined;
    this.info = undefined;
    this.destination = undefined;
    this.title = undefined;
    this.fence_char = undefined;
    this.fence_length = undefined;
    this.fence_offset = undefined;
    this.level = undefined;
};

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
    var walker = NodeWalker(this);
    return walker;
};

var nodeToObject = function(node) {
    var result = {};
    var propsToShow = ['t', 'literal', 'list_data', 'sourcepos',
                       'info', 'level', 'title', 'destination'];

    for (var i = 0, len = propsToShow.length; i < len; i++) {
        var prop = propsToShow[i];
        if (node[prop] !== undefined) {
            result[prop] = node[prop];
        }
    }
    return result;
};

Node.prototype.toObject = function() {
    var childrenStack = [];
    var walker = this.walker();
    var event;
    while ((event = walker.next())) {
        var node = event.node;
        var entering = event.entering;
        var container = node.isContainer();
        var astnode;

        if (container) {
            if (entering) {
                childrenStack.push([]);
            } else {
                astnode = nodeToObject(node);
                astnode.children = childrenStack.pop();
                if (childrenStack.length > 0) {
                    childrenStack[childrenStack.length - 1].push(astnode);
                }
            }
        } else {
            astnode = nodeToObject(node);
            childrenStack[childrenStack.length - 1].push(astnode);
        }
    }

    return astnode;

};

module.exports = Node;


/* Example of use of walker:

 var walker = w.walker();
 var event;

 while (event = walker.next()) {
 console.log(event.entering, event.node.t);
 }

 */
