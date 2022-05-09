#!/usr/bin/perl -W

# aman_add_replication_users.pl
#
#  Add MySQL Replication User Logins for Aman
#
# (C) Copyright 2013-2022 Fred Gleason <fredg@paravelsystems.com>
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

my @addresses;

sub ReadAmanConfig
{
    my $section="";
    my $line="";
    
    my @fields;

    if(!open CONFIG,"<","/etc/aman.conf") {
	die "unable to open configuration file \"/etc/aman.conf\".\n";
    }
    while($line=<CONFIG>) {
	chomp($line);
	if((substr($line,0,1) eq "[")&&
	   (substr($line,length($line)-1,1) eq "]")) {
	    $section=substr($line,1,length($line)-2);
	}
	else {
	    @fields=split "=",$line;
	    if(scalar @fields != 0) {
		if(lc($section) eq "systema"){
		    if(lc($fields[0]) eq "publicaddress") {
			$addresses[0]=$fields[1];
		    }
		    if(lc($fields[0]) eq "privateaddress") {
			$addresses[1]=$fields[1];
		    }
		}
		if(lc($section) eq "systemb"){
		    if(lc($fields[0]) eq "publicaddress") {
			$addresses[2]=$fields[1];
		    }
		    if(lc($fields[0]) eq "privateaddress") {
			$addresses[3]=$fields[1];
		    }
		}
	    }
	}
    }
    $addresses[@addresses]="localhost";

    close CONFIG;
}

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
# Get Defaults
#
ReadAmanConfig();

#
# Prompt for arguments
#
my $mysql_username=GetLine("MySQL Admin User","root");
print "MySQL Admin Password: ";
ReadMode('noecho');
chomp(my $mysql_password = <STDIN>);
ReadMode(0);
print "\n";
my $repl_username=GetLine("Replication User","repl");
my $repl_password=GetLine("Replication Password","letmein");
$addresses[0]=GetLine("System A Public Address",$addresses[0]);
$addresses[1]=GetLine("System A Private Address",$addresses[1]);
$addresses[2]=GetLine("System B Public Address",$addresses[2]);
$addresses[3]=GetLine("System B Private Address",$addresses[3]);

#
# Open Database
#
my $dbh=DBI->connect("dbi:mysql:mysql:localhost",
		     $mysql_username,$mysql_password)
    or die "Couldn't connect to MySQL: " . DBI->errstr;

#
# Remove Stale User Entries
#
my $sql="delete from `user` where `User`='".$repl_username."'";
ExecuteSql($dbh,$sql);

#
# Add Users
#
foreach(@addresses) {
    $sql="insert into `user` set `Host`='".$_."',".
	"`User`='".$repl_username."',".
	"`Password`=password('".$repl_password."'),".
	"`Select_priv`='Y',".
	"`Insert_priv`='Y',".
	"`Update_priv`='Y',".
	"`Create_priv`='Y',".
	"`Delete_priv`='Y',".
	"`Reload_priv`='Y',".
	"`Super_priv`='Y',".
	"`Drop_priv`='Y',".
	"`Repl_slave_priv`='Y',".
	"`Repl_client_priv`='Y'";
    ExecuteSql($dbh,$sql);
}

$sql="flush privileges";
ExecuteSql($dbh,$sql);

#
# Clean Up
#
$dbh->disconnect();
