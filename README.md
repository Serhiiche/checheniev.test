Drupal Middle test case.

P.S. Please evaluate a junior level developer :)
===================================

To get content use:

git clone https://github.com/Serhiiche/checheniev.test.git
----------------------------------

SQL dump is in a root of the site ('checheniev_db.sql').

(current settings.php)
database checheniev_db
username root
password

Access to admin:
login admin
psswd admin1
===================================

Tasks number 1, 2, 4 was fulfilled.
See custom module in 'sites/all/modules/_custom/test_task'.

===================================

Implement as much items as possible. Any extra features are welcomed. 
 
Platform: D​rupal 7 
Result: s​ource code and SQL dump 

1. Create a Drupal module: 
	1.1. Module should define a new content type 
	1.2. Module must use own node templates  
	1.3. Create a block with last 3 nodes (in code) 
	1.4. Create a block with last 5 nodes (via V​iews)​ and export it to your module 
	1.5. Put block from 1.3. i​​nto first sidebar by default 
	1.6. Create separate templates for nodes with odd and even node ids 
	1.7. Add link field to your content type in code (use Link module) 
2. Integrate your module with Webform: 
	2.1. Webform module allows to create lists which can be used for Select/Checkboxes/Radios elements. Create new redefined select list with a list of your nodes (check default C​ountries o​r Day of week l​ists) 
3. Integrate with REST services (h​ttps://drupal.org/sandbox/Taran2L/1807378)​: 
	3.1. Create a new resource /​api/my_node ​with only one method implemented (index) 
4. Integrate your module with Panels 
	4.1. Create custom Panels pane, that displays last 4 nodes 