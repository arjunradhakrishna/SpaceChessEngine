#include "CliAlgo.h"

#include <string>
#include <chess/algo_factory.h>

namespace {
	inline bool isPawnPromotion(const space::Move & move, const space::IBoard & board) {
		using namespace space;
		auto piece = board.getPiece({ move.sourceRank, move.sourceFile });
		if (!piece || piece->pieceType != PieceType::Pawn)
			return false;
		else if (board.whoPlaysNext() == Color::Black
			&& move.destinationRank == 0)
		{
			return true;
		}
		else if (board.whoPlaysNext() == Color::White
			&& move.destinationRank == 7)
		{
			return true;
		}
		else
			return false;
	}

	inline space::PieceType charToPieceType(char ch)
	{
		using namespace space;
		if (ch >= 'A' && ch <= 'Z')
			ch = ch + 'a' - 'A';
		switch (ch)
		{
		case 'q':
			return PieceType::Queen;
		case 'n':
			return PieceType::Knight;
		case 'r':
			return PieceType::Rook;
		case 'b':
			return PieceType::Bishop;
		default:
			throw std::runtime_error(std::string("Unsupported piece type '") + ch + "'");
		}
	}

	void invalidMove(const std::string& message, std::ostream& out)
	{
		out << "Move should be 5 or 5 chars long.\n"
			<< "\t<source file><source rank><target file><target rank>[<pawn promotion>]\n"
			<< "E.g. to move a2 to a4\n"
			<< "\ta2a4\n"
			<< "or to promote white pawn to queen\n"
			<< "\ta7a8q."
			<< std::endl;
		throw std::runtime_error("Invalid move: " + message);
	}

	space::Move parseMove(std::string moveStr, std::ostream& out)
	{
		for (auto& ch : moveStr) ch = std::tolower(ch);
		if (moveStr.size() != 4 && moveStr.size() != 5)
			invalidMove("Move was not 4 or 5 chars long.", out);
		char srcFile = moveStr[0];
		if (srcFile < 'a' || srcFile > 'i') invalidMove(std::string("Source file '") + srcFile + "' has to be between 'a' and 'h'", out);
		char srcRank = moveStr[1];
		if (srcRank < '1' || srcRank > '8') invalidMove(std::string("Source rank '") + srcRank + "' has to be between '1' and '7'", out);
		char tgtFile = moveStr[2];
		if (tgtFile < 'a' || tgtFile > 'i') invalidMove(std::string("Target file '") + tgtFile + "' has to be between 'a' and 'h'", out);
		char tgtRank = moveStr[3];
		if (tgtRank < '1' || tgtRank > '8') invalidMove(std::string("Target rank '") + tgtRank + "' has to be between '1' and '7'", out);

		space::PieceType pieceType = space::PieceType::None;
		if (moveStr.size() == 5) pieceType = charToPieceType(moveStr[4]);

		space::Move move;
		move.sourceRank = srcRank - '1';
		move.sourceFile = srcFile - 'a';
		move.destinationRank = tgtRank - '1';
		move.destinationFile = tgtFile - 'a';
		move.promotedPiece = pieceType;

		return move;
		
	}

	inline char pieceTypeToChar(space::PieceType pieceType)
	{
		switch (pieceType)
		{
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

	std::string moveToString(space::Move move)
	{
		std::string result("     ");
		result[0] = static_cast<char>(move.sourceFile + 'a');
		result[1] = static_cast<char>(move.sourceRank + '1');
		result[2] = static_cast<char>(move.destinationFile + 'a');
		result[3] = static_cast<char>(move.destinationRank + '1');
		if (move.promotedPiece != space::PieceType::None)
			result[4] = pieceTypeToChar(move.promotedPiece);
		return result;
	}


	std::istream& initializeInputStream(const nlohmann::json& config, std::ifstream& fin)
	{
		std::string inputFile = config.value(space::CliAlgo::getInputFileField(), "stdin");
		if (inputFile == "stdin")
			return std::cin;
		else
		{
			fin.open(inputFile);
			return fin;
		}
	}

	std::ostream& initializeOutputStream(const nlohmann::json& config, std::ofstream& fout)
	{
		std::string outputFile = config.value(space::CliAlgo::getOutputFileField(), "stdout");
		if (outputFile == "stdout")
			return std::cout;
		else
		{
			fout.open(outputFile);
			return fout;
		}
	}
}


namespace space{

	CliAlgo::CliAlgo(std::istream& inputStream, std::ostream& outputStream) :
		m_fileInput(),
		m_fileOutput(),
		m_inputStream(inputStream),
		m_outputStream(outputStream)
	{ }

	CliAlgo::CliAlgo(const nlohmann::json& config) :
		m_fileInput(),
		m_fileOutput(),
		m_inputStream(initializeInputStream(config, m_fileInput)),
		m_outputStream(initializeOutputStream(config, m_fileOutput))
	{
		auto inputStream = config["inputStream"];
	}

	Move CliAlgo::getNextMove(IBoard::Ptr board) {
		auto validMoves = board->getValidMoves();
		Move result;
		std::string moveStr;
		do {
			m_outputStream << "Enter move: ";
			m_inputStream >> moveStr;
			result = parseMove(moveStr, m_outputStream);
			if (isPawnPromotion(result, *board))
			{
				m_outputStream << "Enter the piece the pawn is promoted to: ";
				char piece;
				m_inputStream >> piece;
				result.promotedPiece = charToPieceType(piece);
			}
			if (validMoves.count(result) == 0)
			{
				m_outputStream << "Invalid move '" << moveToString(result) << "'. Valid moves are: \n";
				for (const auto& validMove : validMoves)
					m_outputStream << moveToString(validMove.first) << " ";
				m_outputStream << std::endl;
			}
		} while (validMoves.count(result) == 0);
		return result;
	}

	std::string CliAlgo::getAlgoName() { return "CliAlgo"; }
	std::string CliAlgo::getInputFileField() { return "InputFile"; }
	std::string CliAlgo::getOutputFileField() { return "OutputFile"; }
	bool CliAlgo::s_algoMachineRegistration = AlgoFactory::registerAlgoMachine(CliAlgo::getAlgoName(), CliAlgo::createFromConfig);

}
