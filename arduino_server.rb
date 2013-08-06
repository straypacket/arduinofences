##
# Call with:
#  curl -i -H "Accept: application/json" "http://localhost:4567/?foo=1"
# or post with: 
#  curl -X POST -H "Accept: application/json" "http://localhost:4567/?foo=1" --data "foo=2"
##
require 'rubygems'
require 'sinatra'
require 'json'
 
encoding_options = {
  :invalid           => :replace,  # Replace invalid byte sequences
  :undef             => :replace,  # Replace anything not defined in ASCII
  :replace           => '',        # Use a blank for those replacements
  :universal_newline => true       # Always break lines with \n
}

set :bind, '0.0.0.0'
 
get '/' do
  p params["foo"]
end

post '/' do
  p params["foo"]
end