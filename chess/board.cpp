#include "board.h"
#include <vector>
#include <sstream>

namespace {
	using namespace space;

	char as_char(PieceType pieceType) {
		switch (pieceType) {
			case PieceType::Rook  : return 'R';
			case PieceType::Knight: return 'N';
			case PieceType::Bishop: return 'B';
			case PieceType::King  : return 'K';
			case PieceType::Queen : return 'Q';
			case PieceType::Pawn  : return 'P';
			default               : return '-';
		}
	}

	std::string piece_to_unicode(const space::Piece& piece) {
		using namespace space;
		if (piece.color == Color::White) {
			switch (piece.pieceType) {
				case PieceType::King: return "\u2654";
				case PieceType::Queen: return "\u2655";
				case PieceType::Rook: return "\u2656";
				case PieceType::Bishop: return "\u2657";
				case PieceType::Knight: return "\u2658";
				case PieceType::Pawn: return "\u2659";
			}
		}
		else {
			switch (piece.pieceType) {
				case PieceType::King: return "\u265a";
				case PieceType::Queen: return "\u265b";
				case PieceType::Rook: return "\u265c";
				case PieceType::Bishop: return "\u265d";
				case PieceType::Knight: return "\u265e";
				case PieceType::Pawn: return "\u265f";
			}
		}

		// PieceType::None
		return " ";
	}

	inline char pieceTypeToChar(space::PieceType pieceType) {
		switch (pieceType) {
		case space::PieceType::Pawn:
			return 'p';
		case space::PieceType::Rook:
			return 'r';
		case space::PieceType::Knight:
			return 'n';
		case space::PieceType::Bishop:
			return 'b';
		case space::PieceType::Queen:
			return 'q';
		case space::PieceType::King:
			return 'k';
		case space::PieceType::None:
			throw std::runtime_error("Cannot convert piece type 'None' to text");
		default:
			throw std::runtime_error("pieceType " + std::to_string(static_cast<int>(pieceType)) + " not recognized.");
		}
	}

	char pieceToChar(space::Piece piece) {
		char result = pieceTypeToChar(piece.pieceType);
		if (piece.color == space::Color::White)
			result = result + 'A' - 'a';
		return result;
	}

	std::string board_as_plain_string(
		const space::IBoard& board,
		bool unicode_pieces,
		space::Color perspective) {
		std::stringstream out;
		out << "  |  a  b  c  d  e  f  g  h  |\n"
			<< "--+--------------------------+--\n";
		for (int rank = 7; rank >= 0; --rank)
		{
			out << (rank + 1) << " |  ";
			for (int file = 0; file < 8; ++file)
			{
				auto piece = board.getPiece({ rank, file });
				if (piece.has_value()) {
					if (unicode_pieces) {
						out << piece_to_unicode(piece.value()) << "  ";
					}
					else {
						auto c = piece.value().pieceType != space::PieceType::None
						    ? pieceToChar(piece.value())
							: ' ';
						out << c << "  ";
					}
				}
				else {
					out << ".  ";
				}
			}
			out << "| " << (rank + 1) << "\n";
			if (rank > 0)
				out << "  |                          |  \n";
		}
		out << "--+--------------------------+--\n"
			<< "  |  a  b  c  d  e  f  g  h  |\n\n";
		return out.str();
	}

	std::string board_as_colored_string(
		const space::IBoard& board,
		bool unicode_pieces,
		space::Color perspective) {
		int start = perspective == Color::White ? 7 : 0;
		int stop = perspective == Color::White ? -1 : 8;
		int step = start > stop ? -1 : 1;

		auto white_square = "\u001b[47m";
		auto black_square = "\u001b[45m";
		auto reset = "\u001b[0m";

		std::stringstream ss;
		for (int rank = start; rank != stop; rank += step) {
			ss << " " << (rank + 1) << " ";
			for (int file = 7 - start; file != 7 - stop; file -= step) {
				auto square_color = (rank + file) % 2;
				ss << (square_color == 1 ? white_square : black_square);

				auto pieceopt = board.getPiece({ rank, file });
				if (pieceopt.has_value()) {
					auto piece = pieceopt.value();
					if (unicode_pieces)
						ss << "\u001b[30m" << piece_to_unicode(piece);
					else
						ss << "\u001b[30m" << pieceToChar(piece);

				}
				else {
					ss << ' ';
				}
				ss << ' ' << reset;
			}
			ss << std::endl;
		}
		ss << (perspective == Color::White ? "   a b c d e f g h" : "   h g f e d c b a")
		   << std::endl;
		if (board.canCastleRight(Color::White)) {
			ss << "White can short castle" << std::endl;
		}
		if (board.canCastleRight(Color::White)) {
			ss << "White can long castle" << std::endl;
		}
		if (board.canCastleRight(Color::Black)) {
			ss << "Black can long castle" << std::endl;
		}
		if (board.canCastleRight(Color::Black)) {
			ss << "Black can short castle" << std::endl;
		}

		return ss.str();
	}
}

namespace space {
	std::string IBoard::as_string(
			bool terminal_colors,
			bool unicode_pieces,
			Color perspective
	) const {
		if (!terminal_colors)
			return board_as_plain_string(*this, unicode_pieces, perspective);
		else
			return board_as_colored_string(*this, unicode_pieces, perspective);
	}
}
