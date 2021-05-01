#include "fen.h"

#include <stdexcept>
#include <sstream>

namespace {

	char pieceToChar(space::Piece piece)
	{
		char result;
		// Pawn, EnPassantCapturablePawn, Rook, Knight, Bishop, Queen, King, None
		switch (piece.pieceType)
		{
		case space::PieceType::Pawn:
			result = 'P';
			break;
		case space::PieceType::Rook:
			result = 'R';
			break;
		case space::PieceType::Knight:
			result = 'N';
			break;
		case space::PieceType::Bishop:
			result = 'B';
			break;
		case space::PieceType::Queen:
			result = 'Q';
			break;
		case space::PieceType::King:
			result = 'K';
			break;
		default:
			throw std::runtime_error("Unpexpected piece type " + std::to_string(static_cast<int>(piece.pieceType)));
		}
		if (piece.color == space::Color::Black)
			result = (result + ('a' - 'A'));
		return result;
	}

} // end anonymous namespace




namespace space {
	Fen Fen::fromBoard(const IBoard::Ptr& board, int halfMoveClock, int fullMoves)
	{
		std::stringstream result;
		std::optional<Position> enPassantPosition = board->enPassantSquare;
		for (int rank = 7; rank >= 0; --rank)
		{
			Position pos(rank, 1);
			int skip = 0;
			for (int file = 0; file < 8; ++file)
			{
				pos.file = file;
				auto optPiece = board->getPiece(pos);
				if (!optPiece || optPiece->pieceType == PieceType::None) 
				{
					++skip;
				} else
				{
					if (skip > 0)
						result << skip;
					skip = 0;
					auto piece = *optPiece;
					result << pieceToChar(piece);
				}
			}
			if (skip > 0) result << skip;
			if (rank > 0) result << '/';
		}

		result << ' ';
		result << (board->whoPlaysNext() == space::Color::Black ? 'b' : 'w');

		result << ' ';
		bool canCastle = false;
		if (board->canCastleLeft(Color::White)) { result << 'K'; canCastle = true; }
		if (board->canCastleRight(Color::White)) { result << 'Q'; canCastle = true; }
		if (board->canCastleRight(Color::Black)) { result << 'k'; canCastle = true; }
		if (board->canCastleLeft(Color::Black)) { result << 'q'; canCastle = true; }
		if (!canCastle) result << '-';

		result << ' ';
		if (!enPassantPosition)
			result << '-';
		else
		{
			result << static_cast<char>('a' + enPassantPosition->file);
			result << (enPassantPosition->rank + 1);
		}

		result << ' ' << halfMoveClock << ' ' << fullMoves;

		return result.str();
	}


	std::vector<std::string> space::Fen::moves2string(const IBoard::Ptr& board, std::vector<Move> movesList)
	{
		std::vector<std::string> strList;
		for (const Move &m : movesList) {
			strList.push_back(moveStr(board, m));
		}

		return strList;
	}

	std::vector<std::string> Fen::moves2string(const IBoard::Ptr& board, IBoard::MoveMap mv)
	{

		std::vector<std::string> strList;
		for (const auto &m : mv) {
			strList.push_back(moveStr(board, m.first));
		}

		return strList;
	}

	// doesnt check if moves are valid
	std::string Fen::moveStr(const IBoard::Ptr& board, Move m)
	{
		std::string moveStr = "";
		std::optional<Piece> pSource = board->getPiece({ m.sourceRank, m.sourceFile });
		std::optional<Piece> pTarget = board->getPiece({ m.destinationRank,m.destinationFile });

		auto num2char = [](int num) { return char('0' + num);  };

		moveStr += pSource.has_value() ? pieceToChar(pSource.value()) : '-';

		moveStr += num2char(m.sourceRank);
		moveStr += num2char(m.sourceFile);
		moveStr += num2char(m.destinationRank);
		moveStr += num2char(m.destinationFile);

		moveStr += pTarget.has_value() ? pieceToChar(pTarget.value()) : '-';
		if (pSource.has_value()) {
			switch (pSource.value().pieceType) {

			case PieceType::King:
				if (abs(m.sourceFile - m.destinationFile) == 2)
					moveStr += "C";    //Castling
				break;

			case PieceType::Pawn:      // promotion
				if((pSource.value().color == Color::White 
						&& m.sourceRank == 6 && m.destinationRank == 7) ||
					(pSource.value().color == Color::Black
						&& m.sourceRank == 1 && m.destinationRank == 0))
					moveStr += pieceToChar({ m.promotedPiece , pSource.value().color });
				break;

			default:
				break;
			}
		}
		return moveStr;
	}







}
