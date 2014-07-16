#!/usr/bin/perl -W

# aman_add_rivendell_user.pl
#
#  Add a MySQL User to a Rivendell Database Setup
#
# (C) Copyright 2013 Fred Gleason <fredg@paravelsystems.com>
#
#      $Id: aman_add_rivendell_user.pl,v 1.1 2013/06/26 00:50:12 cvs Exp $
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License version 2 as
#   published by the Free Software Foundation.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public
#   License along with this program; if not, write to the Free Software
#   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#

use DBI;
use Term::ReadKey;

sub GetLine
{
    print $_[0];
    if($_[1] ne "") {
	print " [".$_[1]."]";
    }
    print ": ";
    chomp($line = <STDIN>);
    if($line eq "") {
	return $_[1];
    }
    return $line;
}


sub ExecuteSql
{
    my $q=$_[0]->prepare($_[1]);
    $q->execute()
	or die "Unable to execute \"".$_[1]."\": ".DBI->errstr;
    $q->finish();
}


#
# Prompt for arguments
#
my $mysql_username=GetLine("MySQL Admin User","root");

print "MySQL Admin Password: ";
ReadMode('noecho');
chomp(my $mysql_password = <STDIN>);
ReadMode(0);
print "\n";
my $rivendell_username=GetLine("Rivendell User","rduser");
my $rivendell_password=GetLine("Rivendell Password","letmein");
my $hostname=GetLine("Host to Add","%");

#
# Open Database
#
my $dbh=DBI->connect("dbi:mysql:mysql:localhost",
		     $mysql_username,$mysql_password)
    or die "Couldn't connect to MySQL: " . DBI->errstr;

#
# Add User
#
my $sql="insert into user set Host=\"".$hostname."\",".
    "User=\"".$rivendell_username."\",".
    "Password=password(\"".$rivendell_password."\")";
ExecuteSql($dbh,$sql);

$sql="insert into db set Host=\"".$hostname."\",".
    "Db=\"Rivendell\",".
    "User=\"".$rivendell_username."\",".
    "Select_priv=\"Y\",".
    "Insert_priv=\"Y\",".
    "Update_priv=\"Y\",".
    "Delete_priv=\"Y\",".
    "Create_priv=\"Y\",".
    "Drop_priv=\"Y\",".
    "References_priv=\"Y\",".
    "Index_priv=\"Y\",".
    "Alter_priv=\"Y\",".
    "Create_tmp_table_priv=\"Y\",".
    "Lock_tables_priv=\"Y\"";

ExecuteSql($dbh,$sql);

$sql="flush privileges";
ExecuteSql($dbh,$sql);

#
# Clean Up
#
$dbh->disconnect();
