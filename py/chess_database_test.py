from chess_database import ChessDatabase


if __name__ == "__main__":
    d = ChessDatabase('None')
    # text = d.load_game(152)
    # print "".join(text)

    text = """
    Event "Havana ol (Men) fin-A"]
    [Site "Havana"]
    [Date "1966.??.??"]
    [Round "4"]
    [White "Malich, Burkhard"]
    [Black "Jansa, Vlastimil"]
    [Result "1/2-1/2"]
    [ECO "E17"]
    [WhiteElo "2490"]
    [BlackElo "2480"]
    [PlyCount "52"]
    [EventDate "1966.10.25"]
    [EventType "team"]
    [EventRounds "13"]
    [EventCountry "CUB"]
    [Source "ChessBase"]
    [SourceDate "1999.07.01"]
    [WhiteTeam "German Dem Rep"]
    [BlackTeam "Czechoslovakia"]
    [WhiteTeamCountry "DDR"]
    [BlackTeamCountry "CSR"]

    1. d4 Nf6 2. c4 e6 3.Nf3 b6 4.g3 Bb7 5. Bg2 Be7 6. O-O O-O 7. b3 c5 8. Bb2 d5
9. Ne5 Nbd7 10. cxd5 exd5 11. Nd3 Bd6 12. Nc3 Qe7 13. Nf4 Bxf4 14. gxf4 Rad8
15. Rc1 Ne4 16. e3 Qh4 17. Nxe4 dxe4 18. Qe2 cxd4 19. Bxd4 Nf6 20. Rc7 Bc8 21.
Bxf6 gxf6 22. Bxe4 Bg4 23. Qc2 Kh8 24. Qc3 Rg8 25. Kh1 Rg6 26. f3 Qh3 1/2-1/2"""

    game = d.open_game(text.split("\n"))
    assert game.headers == {u'BlackTeamCountry': u'CSR', u'WhiteTeamCountry': u'DDR', u'EventRounds': u'13', u'Source': u'ChessBase', u'ECO': u'E17', u'EventType': u'team', u'EventCountry': u'CUB', u'WhiteElo': u'2490', u'Site': u'Havana', u'BlackElo': u'2480', u'EventDate': u'1966.10.25', u'PlyCount': u'52', u'Black': u'Jansa, Vlastimil', u'Result': u'1/2-1/2', u'BlackTeam': u'Czechoslovakia', u'Date': u'1966.??.??', u'SourceDate': u'1999.07.01', u'White': u'Malich, Burkhard', u'Round': u'4', u'WhiteTeam': u'German Dem Rep'}
    assert game.moves == [u'Event', u'"Havana', u'ol', u'(Men)', u'd4', u'Nf6', u'c4', u'e6', u'Nf3', u'b6', u'g3', u'Bb7', u'Bg2', u'Be7', u'O-O', u'O-O', u'b3', u'c5', u'Bb2', u'Ne5', u'Nbd7', u'cxd5', u'exd5', u'Nd3', u'Bd6', u'Nc3', u'Qe7', u'Nf4', u'Bxf4', u'gxf4', u'Rc1', u'Ne4', u'e3', u'Qh4', u'Nxe4', u'dxe4', u'Qe2', u'cxd4', u'Bxd4', u'Nf6', u'Rc7', u'Bc8', u'Bxf6', u'gxf6', u'Bxe4', u'Bg4', u'Qc2', u'Kh8', u'Qc3', u'Rg8', u'Kh1', u'Rg6', u'f3', u'Qh3']