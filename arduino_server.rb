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
set :logging, :true

before do
  puts '[Params]'
  p params
end

get '/' do
  if params["tmp"] and params["hum"]
  	p params["tmp"]
    p params["hum"]
  else
  	p "Incomplete data"
  end
  return "testing!"
end

post '/' do
  p params["api"]
end