//
// Create 10 "hello" messages, 1 per second, print and discard
//

RatedSource(DATA "hello", RATE 1, LIMIT 10)
						-> Print(Sending)
						-> Discard
