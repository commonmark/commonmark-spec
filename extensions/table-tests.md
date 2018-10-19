## `|␠Foo␉|␠Bar␉ |␠␠` with `|----`

| head	| head	|  
|-----	|-----	|  
| foo1	| bar2	|  
| baz3	| quz4	|  

## `|␠Foo␉|␠Bar␉ |` with `|----`

|head	| head|
|----	| ----|
|foo1	| bar2|
|baz3	| quz4|

## `|␠Foo␉|␠Bar␉ |␠␠` with `|␠----`

| head	| head	|  
| ----	| ----	|  
| foo1	| bar2	|  
| baz3	| quz4	|  

## `|␠Foo␉|␠Bar␉ |` with `|␠----`

| head	| head|
| ----	| ----|
| foo1	| bar2|
| baz3	| quz4|

Cell Separation
===============

## `Foo␉|Bar`

|head	|head|
|----	|----|
|foo1	|bar2|
|baz3	|quz4|

## `Foo|␉Bar`

|head|	head|
|----|	----|
|foo1|	bar2|
|baz3|	quz4|

## `Foo␉|␠Bar`

|head	| head|
|----	| ----|
|foo1	| bar2|
|baz3	| quz4|

## `Foo␠|␉Bar`

|head |	head|
|---- |	----|
|foo1 |	bar2|
|baz3 |	quz4|

## `Foo␉|␉Bar`

|head	|	head|
|----	|	----|
|foo1	|	bar2|
|baz3	|	quz4|

## Single column

### `|Foo␉|`

|head	|
|----	|
|foo1	|
|bar2	|

### `|␉Foo|`

|	head|
|	----|
|	foo1|
|	bar2|

### `|␠Foo␉|`

| head	|
| ----	|
| foo1	|
| bar2	|

### `|␉Foo␠|`

|	head |
|	---- |
|	foo1 |
|	bar2 |

### `|␉Foo␉|`

|	head	|
|	----	|
|	foo1	|
|	bar2	|

## `Foo|Bar`

### `|Foo|Bar|`

|head|head|
|----|----|
|foo1|bar2|
|baz3|quz4|

### `|Foo|Bar`

|head|head
|----|----
|foo1|bar2
|baz3|quz4

### `Foo|Bar|`

head|head|
----|----|
foo1|bar2|
baz3|quz4|

### `Foo|Bar`

head|head
----|----
foo1|bar2
baz3|quz4

## `Foo␉|␠Bar`

### `|Foo␉|␠Bar|`

|head	| head|
|----	| ----|
|foo1	| bar2|
|baz3	| quz4|

### `|Foo␉|␠Bar`

|head	| head
|----	| ----
|foo1	| bar2
|baz3	| quz4

### `Foo␉|␠Bar|`

head	| head|
----	| ----|
foo1	| bar2|
baz3	| quz4|

### `Foo␉|␠Bar`

head	| head
----	| ----
foo1	| bar2
baz3	| quz4

## `Foo␠|␉Bar`

### `|␉Foo␠|␉Bar␠|`

|	head |	head |
|	---- |	---- |
|	foo1 |	bar2 |
|	baz3 |	quz4 |

### `|Foo␠|␉Bar|`

|	head |	head
|	---- |	----
|	foo1 |	bar2
|	baz3 |	quz4

### `|Foo␠|␉Bar|`

head |	head
---- |	----
foo1 |	bar2
baz3 |	quz4

## `Foo␉|␉Bar`

### `|␉Foo␉|␉Bar␉|`

|	head	|	head	|
|	----	|	----	|
|	foo1	|	bar2	|
|	baz3	|	quz4	|

### `|␉Foo␉|␉Bar`

|	head	|	head
|	----	|	----
|	foo1	|	bar2
|	baz3	|	quz4

### `Foo␉|␉Bar`

head	|	head
----	|	----
foo1	|	bar2
baz3	|	quz4

Line Endings
============

All tests in this section have the same `|Foo|Bar` line structure without any whitespace. They only differ in what comes after this part before the end of the line.

## Nothing

|head|head
|----|----
|foo1|bar2
|baz3|quz4

## `|`

|head|head|
|----|----|
|foo1|bar2|
|baz3|quz4|

## `␠`

|head|head 
|----|---- 
|foo1|bar2 
|baz3|quz4 

## `|␠`

|head|head|
|----|----|
|foo1|bar2|
|baz3|quz4|

## `␠␠`

|head|head  
|----|----  
|foo1|bar2  
|baz3|quz4  

## `|␠␠`

|head|head|
|----|----|
|foo1|bar2|
|baz3|quz4|

## `␉`

|head|head	
|----|----	
|foo1|bar2	
|baz3|quz4	

## `|␉`

|head|head|	
|----|----|	
|foo1|bar2|	
|baz3|quz4|	

## `␉|`

|head|head	|
|----|----	|
|foo1|bar2	|
|baz3|quz4	|

## `␉|␠`

|head|head	| 
|----|----	| 
|foo1|bar2	| 
|baz3|quz4	| 

## `␉|␠␠`

|head|head	|  
|----|----	|  
|foo1|bar2	|  
|baz3|quz4	|  
