# ClauExplorer
Using ClauParser/ClauParser, vztpv/ClauScript, and wxWidgets.
 ![alt text]("지우기 테스트.png") 
# Usage
	# line comment, insert..
	$insert = {
		@x = 1 
		@y = {
			z = 0
		} 
		@a = 3

		@provinces = {
			-1 = {
				x = 0
			}
			-2 = {
				x = 1
			}
		}
	}
	$insert = {
		x = 1

		provinces = {
			$ = {
				x = 0
				@y = wow
			}
		}
	}

	# change..
	$update = {
		@x = 2 # @ : target, 2 : set value
		a = 3 # condition
		y = {
			@z = 4 # @ : target.
		}
		provinces = {
			$ = {
				x = 0
				@y = wow2
			}
		}
	}

	# remove..
	$delete = {
		@x = 1 # @ : remove object., if value is 1 then remove
		a = 3 # condition.
		y = {
			@z = %any # %any : condition - always.
		}
		provinces = {
			@$ = { # $ : all usertype( array or object or mixed )
				x = 0 # condition.
			}
		}
	}
