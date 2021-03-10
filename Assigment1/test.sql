create table mytest (
    id integer primary key,
    set intSet);
insert into mytest (id, set) values
(1, '{1,2,3,4,9}');


insert into mytest (id, set) values
(2,'{4,3,2,1}');

insert into mytest (id, set) values
(3,'{1,2,3}');

insert into mytest (id, set) values
(4,'{1,5}');

insert into mytest (id, set) values
(5,'{5}');

insert into mytest (id, set) values
(6,'{}');

select a.set,b.set,a.set!!b.set from mytest a, mytest b where a.id=1 and b.id=4;

select a.set,b.set,a.set-b.set from mytest a, mytest b where a.id=1 and b.id=4;

